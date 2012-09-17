package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

abstract sealed class MethodDetail {
  def jobNameParts: List[String] = Nil
  def orderParams(n: MethodNames): List[(String, String)] = Nil
}

case class LogError(errorCode: String) extends MethodDetail {
  override def orderParams(o: MethodNames) = List(o.logError -> errorCode)
}
