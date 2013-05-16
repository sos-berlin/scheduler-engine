package com.sos.scheduler.engine.newkernel.schedule

import org.joda.time.DateTimeZone.UTC
import org.joda.time.{Instant, DateMidnight, DateTime, LocalTime}

final case class TimeOfDay(millis: Long) extends AnyVal with Ordered[TimeOfDay] {
  //(AnyVal) require(millis >= 0 && millis < 2*24*60*60*1000)

  def compare(o: TimeOfDay) =
    millis compare o.millis

  def -(o: TimeOfDay) =
    new TimeOfDay(millis - o.millis)

  def toInstant(o: DateMidnight) =
    new Instant(o.getMillis + millis)

  override def toString = {
    val hours = millis / (60*60*1000)
    val t = new LocalTime(millis, UTC)
    hours + t.toString(":mm:ss.SSS")
  }
}

object TimeOfDay {
  private val TimeOfDayPattern = """(\d{1,2}):(\d{1,2})(:(\d{1,2}))?""".r

  def apply(o: String): TimeOfDay = o match {
    case TimeOfDayPattern(h, m, _, s) => TimeOfDay(h.toInt, m.toInt, Option(s).getOrElse("0").toInt)
  }

  def apply(o: DateTime) = new TimeOfDay(o.getMillisOfDay)

  def apply(h: Int, m: Int, s: Int) = {
    require(h >= 0 && m >= 0 && s >= 0)
    new TimeOfDay((h*60*60 + m*60 + s)*1000)
  }
}
