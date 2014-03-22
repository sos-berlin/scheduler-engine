package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.job.TaskPersistentState
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.time.CppJodaConversions._
import org.joda.time.Instant

@ForCpp final class Job(protected[this] val cppProxy: JobC, protected val injector: Injector)
extends FileBased
with Sister
with UnmodifiableJob
with JobPersistence {

  type Path = JobPath

  protected val jobSubsystem = injector.apply[JobSubsystem]

  private def schedulerThreadCallQueue =
    injector.apply[SchedulerThreadCallQueue]

  def onCppProxyInvalidated() {}

  def fileBasedType = FileBasedType.job

  def stringToPath(o: String) = JobPath(o)

  def description: String = cppProxy.description

  def state = JobState.valueOf(cppProxy.state_name)

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
