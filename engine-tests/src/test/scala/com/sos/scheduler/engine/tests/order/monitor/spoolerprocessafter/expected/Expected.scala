package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.jobscheduler.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.job.JobState
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected.Expected._
import org.scalactic.Requirements._

final case class Expected(orderStateExpectation: OrderStateExpectation, details: ExpectedDetail*) {

  val jobState = if (details contains JobIsStopped) JobState.stopped else JobState.pending

  val spoolerProcessAfterParameterOption: Option[Boolean] = details collectFirst { case SpoolerProcessAfterParameter(o) ⇒ o }

  val mandatoryMessageCodes: Map[SchedulerLogLevel, Set[String]] = {
    def messages(level: SchedulerLogLevel) = details collect { case MessageCode(`level`, code, Mandatory) ⇒ code }
    (LogLevels map { level ⇒ level → messages(level).toSet } filter { _._2.nonEmpty }).toMap withDefaultValue Set()
  }

  def requireMandatoryMessageCodes(m: Iterable[(SchedulerLogLevel, Iterable[String])]): Unit = {
    val mandatories = m map { case (level, codes) ⇒ level → (codes.toSet filter mandatoryMessageCodes(level)) } filter { _._2.nonEmpty }
    require(mandatories.toMap == mandatoryMessageCodes)
  }

  override def toString = (orderStateExpectation :: details.toList) mkString ","
}

object Expected {
  val LogLevels = List(SchedulerLogLevel.error, SchedulerLogLevel.warning)
}
