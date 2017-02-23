package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.jobscheduler.data.log.SchedulerLogLevel

sealed trait ExpectedDetail

case object JobIsStopped extends ExpectedDetail

final case class SpoolerProcessAfterParameter(value: Boolean) extends ExpectedDetail {
  override def toString = s"spooler_process_after($value)"
}

abstract class MessageCode(val level: SchedulerLogLevel) extends ExpectedDetail {
  val code: String
  val disposition: Disposition
}

object MessageCode {
  def unapply(o: MessageCode) = Some((o.level, o.code, o.disposition))
}

final case class ErrorCode(code: String, disposition: Disposition = Mandatory) extends MessageCode(SchedulerLogLevel.error)

final case class Warning(code: String, disposition: Disposition = Mandatory) extends MessageCode(SchedulerLogLevel.warning)

sealed trait Disposition
case object Mandatory extends Disposition
case object Ignorable extends Disposition
