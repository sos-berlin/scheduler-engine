package com.sos.scheduler.engine.newkernel.schedule.oldruntime

import OldPeriod._
import com.sos.scheduler.engine.newkernel.schedule.TimeOfDay
import org.joda.time.{Instant, Interval, DateMidnight, Duration}

final case class OldPeriod(
    begin: TimeOfDay = Default.begin,
    end: TimeOfDay = Default.end,
    repeat: Option[Duration] = None,
    absoluteRepeat: Option[Duration] = None,
    once: Boolean = Default.once) {

  require(begin >= Default.begin, s"Period.begin should be >= ${Default.begin}: $begin")
  require(end <= Default.end, s"Period.end should be <= ${Default.end}: $end")
  require(begin <= end, s"Period.begin should not be after end: begin=$begin end=$end")
  for (o <- repeat ++ absoluteRepeat) require(o.getMillis > 0, s"repeat should be positive")
  require(!repeat.isEmpty || absoluteRepeat.isEmpty, s"Only one of attributes repeat and absolute_repeat is possible")

  def startNotBefore(t: TimeOfDay): Option[TimeOfDay] =
    if (hasStart && t < begin)
      Some(begin)
    else
      absoluteRepeat map { a =>
        val n = (t.millis + a.getMillis - 1 - begin.millis) / a.getMillis
        TimeOfDay(n * a.getMillis)
      } filter { _ < end }

  def hasStart =
    once || repeat.nonEmpty || absoluteRepeat.nonEmpty

  def duration =
    new Duration(end.millis - begin.millis)

  def contains(o: TimeOfDay) =
    begin >= o && o < end

  def toInterval(date: DateMidnight) =
    new Interval(
      new Instant(date.getMillis + begin.millis),
      new Instant(date.getMillis + end.millis))
}

object OldPeriod {
  private object Default {
    val begin = TimeOfDay(0)
    val end = TimeOfDay(24*60*60*1000)
    val once = false
  }

  case class Builder(
      var begin: TimeOfDay = Default.begin,
      var end: TimeOfDay = Default.end,
      var repeat: Option[Duration] = None,
      var absoluteRepeat: Option[Duration] = None,
      var once: Boolean = Default.once) {

    def toSchedulePeriod = new OldPeriod(begin, end, repeat, absoluteRepeat, once)
  }
}