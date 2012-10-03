package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.configuration.SchedulerDataConstants.eternalMillis
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskPersistent, TaskId, JobPersistent}
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.folder.FileBasedState
import com.sos.scheduler.engine.kernel.persistence.ScalaJPA._
import com.sos.scheduler.engine.kernel.persistence.{JobStore, TaskStore}
import com.sos.scheduler.engine.kernel.util.SchedulerXmlUtils.byteArrayFromCppByteString
import javax.annotation.Nullable
import javax.persistence.EntityManagerFactory
import org.joda.time.DateTime

@ForCpp final class Job(cppProxy: JobC, injector: Injector) extends FileBased with Sister with UnmodifiableJob {

  import Job._

  implicit private lazy val entityManagerFactory = injector.getInstance(classOf[EntityManagerFactory])

  def onCppProxyInvalidated() {}

  def getFileBasedType = FileBasedType.job

  def getPath = JobPath.of(cppProxy.path)

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
    cppProxy.set_state_cmd(c.cppValue)
  }

  @ForCpp @Nullable def tryFetchPersistentState =
    transaction { implicit entityManager =>
      jobStore.tryFetch(getPath).orNull
    }

  @ForCpp def persistState() {
    transaction { implicit entityManager =>
      jobStore.store(persistentState)
    }
  }

  @ForCpp def deletePersistentState() {
    transaction { implicit entityManager =>
      jobStore.delete(getPath)
    }
  }

  private def jobStore = injector.getInstance(classOf[JobStore])

  private def persistentState = new JobPersistent(
    getPath,
    isPermanentlyStopped,
    eternalMillisToNone(cppProxy.next_start_time_millis))

  @ForCpp def persistEnqueuedTask(taskId: Int, enqueueTimeMillis: Long, startTimeMillis: Long, parametersXml: String, xml: String) {
    transaction { implicit entityManager =>
      taskStore.insert(TaskPersistent(
        TaskId(taskId),
        getPath,
        new DateTime(enqueueTimeMillis),
        zeroMillisToNone(startTimeMillis),
        parametersXml,
        xml))
    }
  }

  @ForCpp def deletePersistedTask(taskId: Int) {
    transaction { implicit entityManager =>
      taskStore.delete(TaskId(taskId))
    }
  }

  @ForCpp def loadPersistentTasks() {
    transaction { implicit entityManager =>
      for (t <- taskStore.fetchByJobOrderedByTaskId(getPath)) {
        cppProxy.enqueue_task(t)
      }
    }
  }

  private def taskStore = injector.getInstance(classOf[TaskStore])

  def isPermanentlyStopped = cppProxy.is_permanently_stopped

  override def toString = getClass.getSimpleName + " " + getPath.asString
}

object Job {
  private def eternalMillisToNone(millis: Long): Option[DateTime] = {
    require(millis > 0, "Timestamp from C++ is not greater than zero: "+millis)
    Some(millis) filter { _ != eternalMillis } map { o => new DateTime(o) }
  }

  private def zeroMillisToNone(millis: Long): Option[DateTime] =
    Some(millis) filter { _ != 0 } map { o => new DateTime(o) }
}