package com.sos.scheduler.engine.playground.plugins.jobnet.node

import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Order
import com.sos.scheduler.engine.data.order.OrderState

trait Entrance {
  def state: OrderState

  def apply(o: Order)
}
