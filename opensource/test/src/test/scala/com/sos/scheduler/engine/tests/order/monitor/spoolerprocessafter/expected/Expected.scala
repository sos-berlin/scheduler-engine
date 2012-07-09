package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.data.log.LogLevel

case class Expected(orderStateExpectation: OrderStateExpectation, details: ExpectedDetail*) {
  import Expected._

  def jobState = if (details contains JobIsStopped) JobState.stopped else JobState.pending

  def spoolerProcessAfterParameterOption: Option[Boolean] = details collectFirst { case SpoolerProcessAfterParameter(o) => o }

  def messageCodes: Iterable[(LogLevel, Iterable[String])] = {
    def messages(level: LogLevel) = details collect { case MessageCode(`level`, code) => code }
    logLevels map { level => level -> messages(level).toSet } filter { !_._2.isEmpty }
  }

  override def toString = (orderStateExpectation :: details.toList) mkString ","
}

object Expected {
  val logLevels = List(LogLevel.error, LogLevel.warning)
}
