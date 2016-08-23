package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.{FileBasedObstacle, FileBasedState, FileBasedType}
import com.sos.scheduler.engine.data.job.{JobObstacle, JobOverview, JobPath, JobState, TaskPersistentState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.{inSchedulerThread, schedulerThreadFuture}
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.kernel.time.CppTimeConversions._
import java.time.Instant

@ForCpp
final class Job(
  protected[this] val cppProxy: JobC,
  protected[kernel] val subsystem: JobSubsystem)
extends FileBased
with Sister
with UnmodifiableJob
with JobPersistence {

  type ThisPath = JobPath

  def onCppProxyInvalidated(): Unit = {}

  def fileBasedType = FileBasedType.Job

  def stringToPath(o: String) = JobPath(o)

  private[kernel] override def overview = {
    val state = this.state
    val isInPeriod = this.isInPeriod
    val taskLimit = this.taskLimit
    val runningTasksCount = this.runningTasksCount
    val obstacles: Set[JobObstacle] = {
      import JobObstacle._
      val builder = Set.newBuilder[JobObstacle]
      emptyToNone(fileBasedObstacles) match {
        case Some(o) ⇒ builder += FileBasedObstacles(o)
        case None ⇒
        val isGoodState = Set(JobState.pending, JobState.running)
        if (!isGoodState(state)) {
          builder += BadState(state)
        }
        val taskLimit = this.taskLimit
        if (runningTasksCount >= taskLimit) builder += TaskLimitReached(taskLimit)
        if (!isInPeriod) builder += NoRuntime(nextPossibleStartInstantOption)
      }
      builder.result
    }
    JobOverview(
      path,
      fileBasedState,
      defaultProcessClassPathOption,
      state,
      isInPeriod = isInPeriod,
      taskLimit = taskLimit,
      usedTaskCount = runningTasksCount,
      obstacles)
  }

  private[kernel] def defaultProcessClassPathOption = emptyToNone(cppProxy.default_process_class_path) map ProcessClassPath.apply

  private def isInPeriod = cppProxy.is_in_period

  private def taskLimit = cppProxy.max_tasks

  private def runningTasksCount = cppProxy.running_tasks_count

  def title: String = inSchedulerThread { cppProxy.title }

  def description: String = inSchedulerThread { cppProxy.description }

  def scriptText: String = inSchedulerThread { cppProxy.script_text() }

  private[kernel] def state = JobState.valueOf(cppProxy.state_name)

  def stateText = inSchedulerThread { cppProxy.state_text }

  def needsProcess: Boolean = inSchedulerThread { cppProxy.waiting_for_process }

  protected def nextPossibleStartInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_possible_start_millis)

  protected def nextStartInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_start_time_millis)

  def endTasks(): Unit = inSchedulerThread { setStateCommand(JobStateCommand.endTasks) }

  def setStateCommand(c: JobStateCommand): Unit = {
    schedulerThreadFuture { cppProxy.set_state_cmd(c.cppValue) } (schedulerThreadCallQueue)
  }

  def isPermanentlyStopped = inSchedulerThread { cppProxy.is_permanently_stopped }

  private[job] def enqueueTaskPersistentState(t: TaskPersistentState): Unit = {
    cppProxy.enqueue_taskPersistentState(t)
  }
}

object Job {
  private[kernel] final class Type extends SisterType[Job, JobC] {
    def sister(proxy: JobC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Job(proxy, injector.instance[JobSubsystem])
    }
  }
}
