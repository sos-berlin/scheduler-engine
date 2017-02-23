package com.sos.scheduler.engine.data.jobchain

import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.order.OrderId
import com.sos.scheduler.engine.data.order.OrderKey

final case class JobChainPath(string: String)
extends TypedPath {

  validate()

  def companion = JobChainPath

  def orderKey(o: String): OrderKey = orderKey(OrderId(o))

  def orderKey(o: OrderId): OrderKey = new OrderKey(this, o)
}


object JobChainPath extends TypedPath.Companion[JobChainPath] {

  override def isCommaAllowed = false
}
