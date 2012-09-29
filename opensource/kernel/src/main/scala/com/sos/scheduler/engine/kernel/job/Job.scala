package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistentState
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.folder.FileBasedState
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.eternalMillis
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerTimeZone
import com.sos.scheduler.engine.kernel.util.SchedulerXmlUtils.byteArrayFromCppByteString
import javax.annotation.Nullable
import org.joda.time.DateTime

@ForCpp final class Job(cppProxy: JobC, injector: Injector) extends FileBased with Sister with UnmodifiableJob {

  import Job._

  def onCppProxyInvalidated() {}

  def getFileBasedType = FileBasedType.job

  def getPath = JobPath.of(cppProxy.path)

  def getName = cppProxy.name

  def getFileBasedState = FileBasedState.ofCppName(cppProxy.file_based_state_name)

  /** @return true, wenn das [[com.sos.scheduler.engine.kernel.folder.FileBased]] nach einer Änderung erneut geladen worden ist. */
  def isFileBasedReread = cppProxy.is_file_based_reread

  def getLog = cppProxy.log.getSister

  def getConfigurationXmlBytes = byteArrayFromCppByteString(cppProxy.source_xml)

  def getDescription = cppProxy.description

  def state = JobState.valueOf(cppProxy.state_name)

  def endTasks() {
    setStateCommand(JobStateCommand.endTasks)
  }

  def setStateCommand(c: JobStateCommand) {
    cppProxy.set_state_cmd(c.cppValue)
  }

  @ForCpp @Nullable def tryFetchPersistentState =  persistentStateStore.tryFetch(getPath).orNull

  @ForCpp def persistState() {
    persistentStateStore.store(persistentState)
  }

  @ForCpp def deletePersistentState() {
    persistentStateStore.delete(getPath)
  }

  private def persistentStateStore = injector.getInstance(classOf[JobPersistentStateStore])

  private def persistentState: JobPersistentState =
    new JobPersistentState(
      getPath,
      isPermanentlyStopped,
      eternalMillisToNone(cppProxy.next_start_time_millis))

  def isPermanentlyStopped = cppProxy.is_permanently_stopped

  override def toString = getClass.getSimpleName + " " + getPath.asString
}

object Job {
  private def eternalMillisToNone(millis: Long): Option[DateTime] = {
    require(millis > 0, "Timestamp from C++ is not greater than zero: "+millis)
    Some(millis) filter { _ != eternalMillis } map { o => new DateTime(o, schedulerTimeZone) }
  }
}