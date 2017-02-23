package com.sos.scheduler.engine.kernel.job

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.base.utils.ScalaUtils.SwitchStatement
import com.sos.jobscheduler.common.auth.UserId
import com.sos.jobscheduler.common.guice.GuiceImplicits.RichInjector
import com.sos.jobscheduler.common.scalautil.Collections.emptyToNone
import com.sos.jobscheduler.common.scalautil.SetOnce
import com.sos.jobscheduler.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.job.{JobPath, TaskDetailed, TaskKey, TaskObstacle, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.TaskC
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.processclass.ProcessClass
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.kernel.security.AccessTokenRegister
import com.sos.scheduler.engine.kernel.time.CppTimeConversions
import java.nio.file.Paths
import java.time.Instant

@ForCpp
private[engine] final class Task(
  cppProxy: TaskC,
  taskSubsystem: TaskSubsystem,
  accessTokenRegister: AccessTokenRegister)
extends UnmodifiableTask with Sister with EventSource {

  import taskSubsystem.schedulerThreadCallQueue
  intelliJuseImports(schedulerThreadCallQueue)

  private val taskIdOnce = new SetOnce[TaskId]
  private val webServiceAccessTokenToken = new SetOnce[SecretString]

  def onCppProxyInvalidated(): Unit = {
    webServiceAccessTokenToken foreach accessTokenRegister.remove
  }

  private[kernel] def details = TaskDetailed(
    overview,
    variables,
    stdoutFile = stdoutFile)

  private[kernel] def overview = TaskOverview(taskId, jobPath, state, processClassOption map { _.path }, agentAddress, obstacles)

  private def obstacles: Set[TaskObstacle] = {
    import TaskObstacle._
    val builder = Set.newBuilder[TaskObstacle]
    state switch {
      case TaskState.waiting_for_process if cppProxy.is_waiting_for_remote_scheduler ⇒
        builder += WaitingForAgent
      case TaskState.waiting_for_process ⇒
        builder += WaitingForProcessClass
      case TaskState.opening_waiting_for_locks | TaskState.running_waiting_for_locks ⇒
        builder += WaitingForLock
      case TaskState.running_delayed ⇒
        builder += Delayed
      case TaskState.suspended ⇒
        builder += Suspended
    }
    builder.result
  }

  private[kernel] def taskKey = TaskKey(jobPath, taskId)

  private def jobPath = JobPath(cppProxy.job_path)

  private[kernel] def job = cppProxy.job.getSister

  private[kernel] def agentAddress: Option[AgentAddress] = emptyToNone(cppProxy.remote_scheduler_address) map AgentAddress.apply

  private[kernel] def orderOption: Option[Order] = Option(cppProxy.order.getSister)

  private[kernel] def taskId = taskIdOnce getOrUpdate TaskId(cppProxy.id)

  private[kernel] def processClassPath: ProcessClassPath =
    processClassOption.getOrElse(throw new NoSuchElementException("Task has not ProcessClass yet")).path

  private[kernel] def processClassOption: Option[ProcessClass] =
    cppProxy.process_class_or_null match {
      case null ⇒ None
      case o ⇒ Some(o.getSister)
    }

  def processStartedAt: Option[Instant] = inSchedulerThread { CppTimeConversions.zeroCppMillisToNoneInstant(cppProxy.processStartedAt) }

  private[kernel] def stepOrProcessStartedAt = CppTimeConversions.zeroCppMillisToNoneInstant(cppProxy.stepOrProcessStartedAt)

  private def state: TaskState = TaskState.of(cppProxy.state_name)

  def parameterValue(name: String): String =
    inSchedulerThread {
      cppProxy.params.get_string(name)
    }

  private def variables: Map[String, String] = cppProxy.params.getSister.toMap

  def stdoutFile = inSchedulerThread { Paths.get(cppProxy.stdout_path) }

  def log: PrefixLog = inSchedulerThread { cppProxy.log.getSister }

  @ForCpp
  private def webServiceAccessTokenString: String =
    webServiceAccessToken.string

  private def webServiceAccessToken: SecretString =
    webServiceAccessTokenToken getOrUpdate accessTokenRegister.newAccessTokenForUser(UserId.Anonymous)

  override def toString = s"Task ${taskIdOnce toStringOr ""}".trim
}

object Task {
  private[kernel] object Type extends SisterType[Task, TaskC] {
    def sister(proxy: TaskC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Task(proxy, injector.instance[TaskSubsystem], injector.instance[AccessTokenRegister])
    }
  }
}
