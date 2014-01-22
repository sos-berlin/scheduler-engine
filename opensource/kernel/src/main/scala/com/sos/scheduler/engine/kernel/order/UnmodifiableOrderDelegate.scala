package com.sos.scheduler.engine.kernel.order

final class UnmodifiableOrderDelegate(order: UnmodifiableOrder)
extends UnmodifiableOrder {

  def key =
    order.key

  def id =
    order.id

  def state =
    order.state

  def endState =
    order.endState

  def isSuspended =
    order.isSuspended

  def title =
    order.title

  def jobChain =
    order.jobChain

  def jobChainOption =
    order.jobChainOption

  def parameters =
    order.parameters

  def log =
    order.log

  def nextInstantOption =
    order.nextInstantOption

  override def toString =
    order.toString
}
