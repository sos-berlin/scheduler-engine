package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.cppproxy.TaskC
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.Order
import java.io.File

@ForCpp
final class Task(cppProxy: TaskC) extends UnmodifiableTask with Sister with EventSource {

  def onCppProxyInvalidated(): Unit = {}

  def job =
    cppProxy.job.getSister

  def orderOption: Option[Order] = Option(cppProxy.order.getSister)

  def id =
    new TaskId(cppProxy.id)

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
