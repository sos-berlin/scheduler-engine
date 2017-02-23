package com.sos.scheduler.engine.kernel.job

import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.common.time.JodaJavaTimeConversions.implicits._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.job.{JobPersistentState, TaskPersistentState}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.time.CppTimeConversions._
import java.time.{Duration, Instant}
import javax.annotation.Nullable

private[job] trait JobPersistence {
  this: Job =>

  protected val subsystem: JobSubsystem

  import subsystem.{entityManagerFactory, jobStore, taskStore}

  @ForCpp @Nullable private[job] final def tryFetchPersistentState(): JobPersistentState =
    transaction(entityManagerFactory) { implicit entityManager =>
      jobStore.tryFetch(path).orNull
    }

  @ForCpp private[job] final def persistState(): Unit = {
    transaction(entityManagerFactory) { implicit entityManager =>
      jobStore.store(persistentState)
    }
  }

  @ForCpp private[job] final def deletePersistentState(): Unit = {
    transaction(entityManagerFactory) { implicit entityManager =>
      jobStore.delete(path)
    }
  }

  @ForCpp final def tryFetchAverageStepDuration(): Option[Duration] =
    transaction(entityManagerFactory) { implicit entityManager =>
      jobStore.tryFetchAverageStepDuration(path)
    }

  @ForCpp private[job] final def persistEnqueuedTask(taskId: Int, enqueueTimeMillis: Long, startTimeMillis: Long, parametersXml: String, xml: String): Unit = {
    transaction(entityManagerFactory) { implicit entityManager =>
      taskStore.insert(TaskPersistentState(
        TaskId(taskId),
        path,
        Instant.ofEpochMilli(enqueueTimeMillis),
        zeroCppMillisToNoneInstant(startTimeMillis) map asJodaInstant,
        parametersXml,
        xml))
    }
  }

  @ForCpp private[job] final def deletePersistedTask(taskId: Int): Unit = {
    transaction(entityManagerFactory) { implicit entityManager =>
      taskStore.delete(TaskId(taskId))
    }
  }

  @ForCpp private[job] final def loadPersistentTasks(): Unit = {
    transaction(entityManagerFactory) { implicit entityManager =>
      taskStore.fetchByJobOrderedByTaskId(path) foreach enqueueTaskPersistentState
    }
  }

  private def persistentState = JobPersistentState(
    path,
    isPermanentlyStopped,
    nextStartInstantOption)
}
