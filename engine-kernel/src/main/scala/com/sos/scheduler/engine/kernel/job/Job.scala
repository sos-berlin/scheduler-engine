package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.base.utils.ScalaUtils.SwitchStatement
import com.sos.scheduler.engine.base.utils.ScalazStyle._
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.job.{JobDescription, JobDetailed, JobObstacle, JobOverview, JobPath, JobState, JobView, TaskPersistentState}
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.{inSchedulerThread, schedulerThreadFuture}
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.job.Job._
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.kernel.time.CppTimeConversions._
import java.time.Instant
import scala.collection.JavaConversions._
import scala.collection.immutable

@ForCpp
final class Job(
  protected[this] val cppProxy: JobC,
  protected[kernel] val subsystem: JobSubsystem,
  protected[kernel] val processClassSubsystem: ProcessClassSubsystem)
extends FileBased
with Sister
with UnmodifiableJob
with JobPersistence {

  protected type Self = Job
  type ThisPath = JobPath

  def onCppProxyInvalidated(): Unit = {}

  def stringToPath(o: String) = JobPath(o)

  private[kernel] def isReadyForOrderIn(processClassPathOption: Option[ProcessClassPath]) =
    (processClassPathOption flatMap processClassSubsystem.fileBasedOption exists { o ⇒ cppProxy.is_task_ready_for_order(o.cppProxy.cppReference) }) ||
      obstacles.isEmpty

  private[kernel] def view[V <: JobView: JobView.Companion]: V =
    implicitly[JobView.Companion[V]] match {
      case JobOverview ⇒ jobOverview
      case JobDetailed ⇒ jobDetailed
      case JobDescription ⇒ jobDescription
    }

  private def jobDetailed: JobDetailed = {
    val tasks = cppProxy.java_tasks.toVector
    val (runningTasks, queuedTasks) = tasks partition (_.startedAt.isDefined)
    JobDetailed(
      jobOverview,
      cppProxy.default_params.getSister.toMap,
      queuedTasks map { t ⇒ JobDetailed.QueuedTask(t.taskId, t.enqueuedAt, t.at) },
      runningTasks map { t ⇒
        JobDetailed.RunningTask(
          t.taskId, t.causeString, t.startedAt.get, t.pid, t.stepCount,
          (t.orderOption, t.nodeKeyOption) match {
            case (Some(o), Some(n)) ⇒ Some(JobDetailed.TaskOrder(o.id, n.jobChainPath, n.nodeId))
            case _ ⇒ None
          }
        )
      })
  }

  private def jobOverview: JobOverview = {
    val state = this.state
    val isInPeriod = this.isInPeriod
    val taskLimit = this.taskLimit
    val runningTasksCount = this.runningTasksCount
    JobOverview(
      path,
      fileBasedState,
      defaultProcessClassPathOption,
      isOrderJob = isOrderControlled,
      title,
      enabled = isEnabled,
      state,
      stateText,
      isInPeriod = isInPeriod,
      nextStartTime = nextStartInstantOption,
      taskLimit = taskLimit,
      usedTaskCount = runningTasksCount,
      queuedTaskCount = cppProxy.queued_task_count,
      lateTaskCount = cppProxy.late_task_count,
      obstacles(state, isInPeriod, taskLimit, runningTasksCount),
      cppProxy.has_error option JobOverview.Error(cppProxy.error_code, cppProxy.error_message))
  }

  private def jobDescription: JobDescription =
    JobDescription(path, description)

  private def obstacles: Set[JobObstacle] = obstacles(state, isInPeriod, taskLimit, runningTasksCount)

  private def obstacles(state: JobState, isInPeriod: Boolean, taskLimit: Int, runningTaskCount: Int): Set[JobObstacle] = {
    import JobObstacle._
    val builder = Set.newBuilder[JobObstacle]
    emptyToNone(fileBasedObstacles) match {
      case Some(o) ⇒
        builder += FileBasedObstacles(o)
      case None ⇒
        if (state == JobState.stopped) builder += Stopped
        else if (!IsGoodState(state)) builder += BadState(state)
        if (runningTasksCount >= taskLimit) builder += TaskLimitReached(taskLimit)
        if (!isInPeriod) builder += NoRuntime(nextPossibleStartInstantOption)
    }
    unavailableLockPaths switch {
      case o if o.nonEmpty ⇒ builder += WaitingForLocks(o)
    }
    if (waitingForProcessClass) {
      builder += WaitingForProcessClass
    }
    for (path ← defaultProcessClassPathOption;
         processClass ← processClassSubsystem.fileBasedOption(path);
         o = processClass.obstacles if o.nonEmpty) {
      builder += ProcessClassObstacles(o)
    }
    builder.result
  }

  private[kernel] lazy val defaultProcessClassPathOption = emptyToNone(cppProxy.default_process_class_path) map ProcessClassPath.apply

  private lazy val isOrderControlled = cppProxy.is_order_controlled

  private def isEnabled = cppProxy.enabled

  private def isInPeriod = cppProxy.is_in_period

  private lazy val taskLimit = cppProxy.max_tasks

  private def runningTasksCount = cppProxy.running_tasks_count

  lazy val title: String = inSchedulerThread { cppProxy.title }

  private lazy val description: String = cppProxy.description

  def scriptText: String = inSchedulerThread { cppProxy.script_text }

  private[kernel] def state = JobState.valueOf(cppProxy.state_name)

  def stateText = inSchedulerThread { cppProxy.state_text }

  private[kernel] def tasks: immutable.Seq[Task] =
    cppProxy.java_tasks.toVector

  private[kernel] def waitingForProcessClass: Boolean = cppProxy.waiting_for_process

  protected def nextPossibleStartInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_possible_start_millis)

  protected def nextStartInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_start_time_millis)

  private def unavailableLockPaths: Set[LockPath] =
    (cppProxy.unavailable_lock_path_strings map LockPath.apply).toSet

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
  private[kernel] object Type extends SisterType[Job, JobC] {
    def sister(proxy: JobC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Job(proxy, injector.instance[JobSubsystem], injector.instance[ProcessClassSubsystem])
    }
  }

  private val IsGoodState = Set(JobState.pending, JobState.running, JobState.stopped)
}
