package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.cppproxy.TaskC
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder
import javax.annotation.Nullable

final class Task(cppProxy: TaskC) extends UnmodifiableTask with Sister with EventSource {

  def onCppProxyInvalidated() {}

  def job =
    cppProxy.job.getSister

  @Nullable def getOrderOrNull: UnmodifiableOrder =
    cppProxy.order.getSister

  def id =
    new TaskId(cppProxy.id)

  def parameterValue(name: String): String =
    cppProxy.params.get_string(name)

  override def toString =
    s"Task($id)"
}
