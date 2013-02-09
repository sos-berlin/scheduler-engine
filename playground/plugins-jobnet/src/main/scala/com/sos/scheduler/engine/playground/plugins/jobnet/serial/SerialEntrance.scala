package com.sos.scheduler.engine.playground.plugins.jobnet.serial

import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Order
import com.sos.scheduler.engine.playground.plugins.jobnet.node.Entrance
import com.sos.scheduler.engine.data.order.OrderState

case class SerialEntrance(state: OrderState) extends Entrance {
  def apply(o: Order) {
    o.moveTo(state)
  }
}
