package com.sos.scheduler.engine.playground.plugins.jobnet.scheduler

import com.sos.scheduler.engine.data.order.{OrderId, OrderState}

final case class Order(jobChain: JobChain, id: OrderId, var state: OrderState) {
  def moveTo(o: OrderState) = ???
  def end() = ???
  def removeFromJobChain() = ???
  def companionStop() = ???
  def companionContinue() = ???
  def cloneOrder(newOrderId: OrderId): Order = cloneOrder(newOrderId, state)
  def cloneOrder(newOrderId: OrderId, newState: OrderState) = Order(jobChain, newOrderId, newState)
}
