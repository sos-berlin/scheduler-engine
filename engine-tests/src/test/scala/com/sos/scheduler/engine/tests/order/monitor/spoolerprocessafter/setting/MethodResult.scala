package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

abstract sealed class MethodResult {
  def orderParams(o: MethodNames): List[(String, String)]
}

case object Throw extends MethodResult {
  override def orderParams(o: MethodNames) = List(o.throwException -> "true")
}

case class Returns(value: Boolean) extends MethodResult {
  override def orderParams(o: MethodNames) = List(o.returns -> value.toString)
}
