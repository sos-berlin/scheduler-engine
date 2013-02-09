package com.sos.scheduler.engine.playground.plugins.jobnet.node

import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Order

trait Exit {
  def orderStates: Set[OrderState]
  def moveOrder(order: Order)
}
