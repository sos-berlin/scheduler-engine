package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

abstract sealed class SettingDetail {
  def jobNameParts: List[String] = Nil
  def orderParams: List[(String, String)] = Nil
}

case object DontStopOnError extends SettingDetail {
  override def jobNameParts = List("DontStopOnError")
  override def toString = "stop_on_error='false'"
}

case class Shell(exitCode: ExitCode) extends SettingDetail {
  override def jobNameParts = "Shell" :: exitCode.jobNameParts
}

case class ExitCode(value: Int) {
  def jobNameParts = List("Exit"+ value)
  override def toString = "exit "+value
}

abstract class Method(val m: MethodNames, result: MethodResult, details: MethodDetail*) extends SettingDetail {
  override def jobNameParts = m.name :: (details.toList flatMap { _.jobNameParts })
  override def orderParams = result.orderParams(m) ++ (details flatMap { _.orderParams(m) })
  override def toString = getClass.getSimpleName + (result :: details.toList).mkString("(", ",", ")")
}

case class SpoolerProcess(result: MethodResult, details: MethodDetail*) extends Method(SpoolerProcessNames, result, details: _*)

case class SpoolerProcessAfter(result: MethodResult, details: MethodDetail*) extends Method(SpoolerProcessAfterNames, result, details: _*)