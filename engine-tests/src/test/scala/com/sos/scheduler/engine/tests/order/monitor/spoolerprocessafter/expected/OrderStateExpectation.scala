package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting.Setting

sealed trait OrderStateExpectation {
  def matches(o: OrderState): Boolean
}

case object InitialState extends OrderStateExpectation {
  def matches(o: OrderState) = o.string startsWith Setting.InitialOrderStatePrefix
}

case object SuccessState extends OrderStateExpectation {
  def matches(o: OrderState) = o == OrderState("SUCCESS")
}

case object ErrorState extends OrderStateExpectation {
  def matches(o: OrderState) = o == OrderState("ERROR")
}
