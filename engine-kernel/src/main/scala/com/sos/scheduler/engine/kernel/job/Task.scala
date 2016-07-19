package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.job.{JobPath, TaskId, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.TaskC
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import java.nio.file.Paths

@ForCpp
private[engine] final class Task(
  cppProxy: TaskC,
  taskSubsystem: TaskSubsystem)
extends UnmodifiableTask with Sister with EventSource {

  import taskSubsystem.schedulerThreadCallQueue
  intelliJuseImports(schedulerThreadCallQueue)

  def onCppProxyInvalidated(): Unit = {}

  private[kernel] def overview = TaskOverview(id, jobPath, state, processClassPath, agentAddress)

  private def jobPath = JobPath(cppProxy.job_path)

  private[kernel] def job = cppProxy.job.getSister

  private[kernel] def agentAddress: Option[AgentAddress] = emptyToNone(cppProxy.remote_scheduler_address) map AgentAddress.apply

  private[kernel] def orderOption: Option[Order] = Option(cppProxy.order.getSister)

  private[kernel] def id = new TaskId(cppProxy.id)

  private[kernel] def processClassPath = ProcessClassPath(cppProxy.process_class_path)

  private def state: TaskState = TaskState.of(cppProxy.state_name)

  def parameterValue(name: String): String =
    inSchedulerThread {
      cppProxy.params.get_string(name)
    }

  def stdoutFile = inSchedulerThread { Paths.get(cppProxy.stdout_path) }

  private[kernel] def log: PrefixLog = inSchedulerThread { cppProxy.log.getSister }

  override def toString =
    s"Task($id)"
}

object Task {
  final class Type extends SisterType[Task, TaskC] {
    def sister(proxy: TaskC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Task(proxy, injector.instance[TaskSubsystem])
    }
  }
}
