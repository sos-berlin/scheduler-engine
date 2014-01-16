package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.job.{TaskId, TaskPersistentState, JobPersistentState}
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateTaskStore, HibernateJobStore}
import com.sos.scheduler.engine.kernel.time.CppJodaConversions._
import javax.annotation.Nullable
import javax.persistence.EntityManager
import org.joda.time.{Duration, Instant}

private[job] trait JobPersistence {
  this: Job =>

  protected def injector: Injector
  protected def cppProxy: JobC

  protected def nextStartInstantOption: Option[Instant]

  @ForCpp @Nullable private[job] def tryFetchPersistentState: JobPersistentState =
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.tryFetch(path).orNull
    }

  @ForCpp private[job] def persistState() {
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.store(persistentState)
    }
  }

  @ForCpp private[job] def deletePersistentState() {
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.delete(path)
    }
  }

  @ForCpp def tryFetchAverageStepDuration(): Option[Duration] =
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.tryFetchAverageStepDuration(path)
    }

  private def persistentStateStore =
    injector.getInstance(classOf[HibernateJobStore])

  private def persistentState = JobPersistentState(
      path,
      isPermanentlyStopped,
      nextStartInstantOption)

  @ForCpp private[job] def persistEnqueuedTask(taskId: Int, enqueueTimeMillis: Long, startTimeMillis: Long, parametersXml: String, xml: String) {
    transaction(entityManager) { implicit entityManager =>
      taskStore.insert(TaskPersistentState(
        TaskId(taskId),
        path,
        new Instant(enqueueTimeMillis),
        zeroMillisToNone(startTimeMillis),
        parametersXml,
        xml))
    }
  }

  @ForCpp private[job] def deletePersistedTask(taskId: Int) {
    transaction(entityManager) { implicit entityManager =>
      taskStore.delete(TaskId(taskId))
    }
  }

  @ForCpp private[job] def loadPersistentTasks() {
    transaction(entityManager) { implicit entityManager =>
      for (t <- taskStore.fetchByJobOrderedByTaskId(path)) {
        cppProxy.enqueue_taskPersistentState(t)
      }
    }
  }

  private def entityManager =
    injector.getInstance(classOf[EntityManager])

  private def taskStore =
    injector.getInstance(classOf[HibernateTaskStore])
}
