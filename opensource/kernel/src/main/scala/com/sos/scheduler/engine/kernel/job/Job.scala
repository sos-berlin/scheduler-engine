package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.folder.FileBasedState
import com.sos.scheduler.engine.kernel.time.CppJodaConversions._
import org.joda.time.Instant
import com.sos.scheduler.engine.data.job.{TaskPersistentState, JobPersistentState}

@ForCpp final class Job(protected val cppProxy: JobC, protected val injector: Injector)
extends FileBased
with Sister
with UnmodifiableJob
with JobPersistence {

  protected val jobSubsystem = injector.apply[JobSubsystem]

  private def schedulerThreadCallQueue =
    injector.apply[SchedulerThreadCallQueue]

  def onCppProxyInvalidated() {}

  def fileBasedType =
    FileBasedType.job

  def path =
    JobPath(cppProxy.path)

  def name =
    cppProxy.name

  def fileBasedState =
    FileBasedState.ofCppName(cppProxy.file_based_state_name)

  /** @return true, wenn das [[com.sos.scheduler.engine.kernel.folder.FileBased]] nach einer Änderung erneut geladen worden ist. */
  def fileBasedIsReread =
    cppProxy.is_file_based_reread

  def log =
    cppProxy.log.getSister

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.folder.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def forceFileReread() {
    cppProxy.set_force_file_reread()
  }

  def configurationXmlBytes =
    cppProxy.source_xml_bytes

  def description =
    cppProxy.description

  def state =
    JobState.valueOf(cppProxy.state_name)

  protected def nextStartInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_start_time_millis)

  def endTasks() {
    setStateCommand(JobStateCommand.endTasks)
  }

  def setStateCommand(c: JobStateCommand) {
    schedulerThreadFuture { cppProxy.set_state_cmd(c.cppValue) } (schedulerThreadCallQueue)
  }

  def isPermanentlyStopped =
    cppProxy.is_permanently_stopped

  private[job] def enqueueTaskPersistentState(t: TaskPersistentState) {
    cppProxy.enqueue_taskPersistentState(t)
  }
}
