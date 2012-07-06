package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.scheduler.engine.data.log.LogLevel

abstract sealed class ExpectedDetail

case object JobIsStopped extends ExpectedDetail

case class SpoolerProcessAfterParameter(value: Boolean) extends ExpectedDetail {
  override def toString = "spooler_process_after("+ value +")"
}

abstract class MessageCode(val level: LogLevel) extends ExpectedDetail {
  val code: String
}

object MessageCode {
  def unapply(o: MessageCode) = Some(o.level, o.code)
}

case class ErrorCode(code: String) extends MessageCode(LogLevel.error)

case class Warning(code: String) extends MessageCode(LogLevel.warning)

