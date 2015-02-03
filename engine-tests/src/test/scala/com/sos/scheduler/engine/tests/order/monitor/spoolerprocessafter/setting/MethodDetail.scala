package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

sealed trait MethodDetail {
  def jobNameParts: List[String] = Nil
  def orderParams(n: MethodNames): List[(String, String)] = Nil
}

final case class LogError(errorCode: String) extends MethodDetail {
  override def orderParams(o: MethodNames) = List(o.logError -> errorCode)
}
