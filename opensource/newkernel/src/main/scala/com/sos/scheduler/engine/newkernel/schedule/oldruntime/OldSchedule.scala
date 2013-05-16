package com.sos.scheduler.engine.newkernel.schedule.oldruntime

import com.sos.scheduler.engine.newkernel.schedule.{TimeOfDay, Weekday, Schedule}
import org.joda.time._
import scala.collection.mutable
import scala.sys.error

class OldSchedule private(
    timeZone: DateTimeZone,
    startOnce: Boolean,
    defaultPeriods: PeriodSeq,
    weekdays: Map[Weekday, PeriodSeq])
extends Schedule {

  def nextInstant(from: Instant): Option[Instant] = {
    val (date, timeOfDay) = splitDateAndTime(from)
    val result = periodSeq(date).nextTimeOfDay(timeOfDay)
    result map { _.toInstant(date) }
  }

  def nextInterval(from: Instant): Option[Interval] = {
    val (date, timeOfDay) = splitDateAndTime(from)
    periodSeq(date).nextPeriod(timeOfDay) map { _.toInterval(date) }
  }

  private def periodSeq(date: DateMidnight) =
    weekdays.getOrElse(Weekday(date.getDayOfWeek), defaultPeriods)

  def splitDateAndTime(o: Instant): (DateMidnight, TimeOfDay) = {
    val dt = new DateTime(o, timeZone)
    (dt.toDateMidnight, TimeOfDay(dt.getMillisOfDay))
  }
}

object OldSchedule {
  class Builder(
      var timeZone: DateTimeZone = null,
      val periods: mutable.Buffer[OldPeriod] = mutable.Buffer(),
      val weekdaysPeriods: mutable.HashMap[Weekday, PeriodSeq] = mutable.HashMap(),
      //val startInstants: mutable.Buffer[Instant] = mutable.Buffer(),
      var startOnce: Boolean = false) {

    def toRuntimeSchedule = new OldSchedule(
      Option(timeZone) getOrElse error("Missing time zone"),
      startOnce,
      PeriodSeq(periods),
      weekdaysPeriods.toMap)
  }
}
