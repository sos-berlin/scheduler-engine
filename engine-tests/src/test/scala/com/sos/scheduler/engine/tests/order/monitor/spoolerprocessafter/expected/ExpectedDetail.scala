package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.scheduler.engine.data.log.SchedulerLogLevel

sealed trait ExpectedDetail

case object JobIsStopped extends ExpectedDetail

final case class SpoolerProcessAfterParameter(value: Boolean) extends ExpectedDetail {
  override def toString = s"spooler_process_after($value)"
}

abstract class MessageCode(val level: SchedulerLogLevel) extends ExpectedDetail {
  val code: String
}

object MessageCode {
  def unapply(o: MessageCode) = Some((o.level, o.code))
}

final case class ErrorCode(code: String) extends MessageCode(SchedulerLogLevel.error)

final case class Warning(code: String) extends MessageCode(SchedulerLogLevel.warning)
