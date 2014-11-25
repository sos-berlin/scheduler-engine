package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected.Expected._

case class Expected(orderStateExpectation: OrderStateExpectation, details: ExpectedDetail*) {

  def jobState = if (details contains JobIsStopped) JobState.stopped else JobState.pending

  def spoolerProcessAfterParameterOption: Option[Boolean] = details collectFirst { case SpoolerProcessAfterParameter(o) => o }

  def messageCodes: Iterable[(SchedulerLogLevel, Iterable[String])] = {
    def messages(level: SchedulerLogLevel) = details collect { case MessageCode(`level`, code) => code }
    LogLevels map { level => level -> messages(level).toSet } filter { _._2.nonEmpty }
  }

  override def toString = (orderStateExpectation :: details.toList) mkString ","
}

object Expected {
  val LogLevels = List(SchedulerLogLevel.error, SchedulerLogLevel.warning)
}
