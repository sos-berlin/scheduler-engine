package com.sos.scheduler.engine.playground.plugins.jobnet.serial

import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Order
import com.sos.scheduler.engine.playground.plugins.jobnet.node.Exit

final case class SerialExit(orderState: OrderState) extends Exit {
  def orderStates = Set(orderState)

  def moveOrder(o: Order) {
    o.moveTo(orderState)
  }
}
