package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.configuration.SchedulerDataConstants.eternalMillis
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskPersistent, TaskId, JobPersistent}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.folder.FileBasedState
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateJobStore, HibernateTaskStore}
import com.sos.scheduler.engine.kernel.util.SchedulerXmlUtils.byteArrayFromCppByteString
import javax.annotation.Nullable
import javax.persistence.EntityManager
import org.joda.time.DateTime

@ForCpp final class Job(cppProxy: JobC, injector: Injector) extends FileBased with Sister with UnmodifiableJob {

  import Job._

  def onCppProxyInvalidated() {}

  implicit private def schedulerThreadCallQueue = injector.instance[SchedulerThreadCallQueue]

  def getFileBasedType = FileBasedType.job

  def getPath = JobPath(cppProxy.path)

  def getName = cppProxy.name

  def getFileBasedState = FileBasedState.ofCppName(cppProxy.file_based_state_name)

  /** @return true, wenn das [[com.sos.scheduler.engine.kernel.folder.FileBased]] nach einer Änderung erneut geladen worden ist. */
  def isFileBasedReread = cppProxy.is_file_based_reread

  def getLog = cppProxy.log.getSister

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.folder.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def forceFileReread() {
    cppProxy.set_force_file_reread()
  }

  def getConfigurationXmlBytes = byteArrayFromCppByteString(cppProxy.source_xml)

  def getDescription = cppProxy.description

  def state = JobState.valueOf(cppProxy.state_name)

  def endTasks() {
    setStateCommand(JobStateCommand.endTasks)
  }

  def setStateCommand(c: JobStateCommand) {
    schedulerThreadFuture { cppProxy.set_state_cmd(c.cppValue) }
  }

  @ForCpp @Nullable private def tryFetchPersistentState =
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.tryFetch(getPath).orNull
    }

  @ForCpp private def persistState() {
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.store(persistentState)
    }
  }

  @ForCpp private def deletePersistentState() {
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.delete(getPath)
    }
  }

  @ForCpp def tryFetchAverageStepDuration() = {
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.tryFetchAverageStepDuration(getPath)
    }
  }
  private def persistentStateStore = injector.getInstance(classOf[HibernateJobStore])

  private def persistentState = new JobPersistent(
    getPath,
    isPermanentlyStopped,
    eternalMillisToNone(cppProxy.next_start_time_millis))

  @ForCpp private def persistEnqueuedTask(taskId: Int, enqueueTimeMillis: Long, startTimeMillis: Long, parametersXml: String, xml: String) {
    transaction(entityManager) { implicit entityManager =>
      taskStore.insert(TaskPersistent(
        TaskId(taskId),
        getPath,
        new DateTime(enqueueTimeMillis),
        zeroMillisToNone(startTimeMillis),
        parametersXml,
        xml))
    }
  }

  @ForCpp private def deletePersistedTask(taskId: Int) {
    transaction(entityManager) { implicit entityManager =>
      taskStore.delete(TaskId(taskId))
    }
  }

  @ForCpp private def loadPersistentTasks() {
    transaction(entityManager) { implicit entityManager =>
      for (t <- taskStore.fetchByJobOrderedByTaskId(getPath)) {
        cppProxy.enqueue_task(t)
      }
    }
  }

  private def entityManager = injector.getInstance(classOf[EntityManager])

  private def taskStore = injector.getInstance(classOf[HibernateTaskStore])

  def isPermanentlyStopped = cppProxy.is_permanently_stopped

  override def toString = getClass.getSimpleName + " " + getPath.string
}

object Job {
  private def eternalMillisToNone(millis: Long): Option[DateTime] = {
    require(millis > 0, "Timestamp from C++ is not greater than zero: "+millis)
    Some(millis) filter { _ != eternalMillis } map { o => new DateTime(o) }
  }

  private def zeroMillisToNone(millis: Long): Option[DateTime] =
    Some(millis) filter { _ != 0 } map { o => new DateTime(o) }
}