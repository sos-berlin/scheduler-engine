package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.job.{JobPath, TaskId, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.cppproxy.TaskC
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.Order
import java.io.File

@ForCpp
private[engine] final class Task(cppProxy: TaskC) extends UnmodifiableTask with Sister with EventSource {

  def onCppProxyInvalidated(): Unit = {}

  def overview = TaskOverview(id, jobPath, state, processClassPath, agentAddress)

  def jobPath = JobPath(cppProxy.job_path)

  def job = cppProxy.job.getSister

  def agentAddress: Option[AgentAddress] = emptyToNone(cppProxy.remote_scheduler_address) map AgentAddress.apply

  def orderOption: Option[Order] = Option(cppProxy.order.getSister)

  def id = new TaskId(cppProxy.id)

  def processClassPath = ProcessClassPath(cppProxy.process_class_path)

  def agentAdress = AgentAddress(cppProxy.remote_scheduler_address)

  def state: TaskState = TaskState.of(cppProxy.state_name)

  def parameterValue(name: String): String =
    cppProxy.params.get_string(name)

  def logString =
    cppProxy.log_string

  def stdoutFile =
    new File(cppProxy.stdout_path)

  def stderrFile =
    new File(cppProxy.stderr_path)

  def log: PrefixLog = cppProxy.log.getSister

  override def toString =
    s"Task($id)"
}
