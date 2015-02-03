package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

sealed trait MethodResult {
  def orderParams(o: MethodNames): List[(String, String)]
}

case object Throw extends MethodResult {
  override def orderParams(o: MethodNames) = List(o.throwException → "true")
}

final case class Returns(value: Boolean) extends MethodResult {
  override def orderParams(o: MethodNames) = List(o.returns → value.toString)
}
