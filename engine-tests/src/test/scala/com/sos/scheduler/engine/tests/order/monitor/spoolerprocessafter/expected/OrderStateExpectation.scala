package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting.Setting

sealed trait OrderStateExpectation {
  def matches(o: NodeId): Boolean
}

case object InitialState extends OrderStateExpectation {
  def matches(o: NodeId) = o.string startsWith Setting.InitialOrderStatePrefix
}

case object SuccessState extends OrderStateExpectation {
  def matches(o: NodeId) = o == NodeId("SUCCESS")
}

case object ErrorState extends OrderStateExpectation {
  def matches(o: NodeId) = o == NodeId("ERROR")
}
