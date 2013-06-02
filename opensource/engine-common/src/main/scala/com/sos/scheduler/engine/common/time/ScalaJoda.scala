package com.sos.scheduler.engine.common.time

import org.joda.time.Duration.{millis, standardSeconds, standardHours, standardDays}
import org.joda.time._
import scala.annotation.tailrec

object ScalaJoda {
  @volatile var extraSleepCount = 0L

  implicit class DurationRichInt(val delegate: Int) extends AnyVal {
    final def ms = millis(delegate)
    final def s = standardSeconds(delegate)
    final def hours = standardHours(delegate)
    final def days = standardDays(delegate)
    final def *(o: Duration) = millis(delegate * o.getMillis)
  }

  implicit class DurationRichLong(val delegate: Long) extends AnyVal {
    final def ms = millis(delegate)
    final def s = standardSeconds(delegate)
    final def hours = standardHours(delegate)
    final def days = standardDays(delegate)
    final def *(o: Duration) = millis(delegate * o.getMillis)
  }

  implicit class RichDuration(val delegate: Duration) extends AnyVal {
    def +(o: Duration) = delegate plus o
    def -(o: Duration) = delegate minus o
    def toScalaDuration = scala.concurrent.duration.Duration(delegate.getMillis, scala.concurrent.duration.MILLISECONDS)
  }

  implicit class RichInstant(val delegate: Instant) extends AnyVal {
    def +(o: Duration) = delegate plus o
    def -(o: Duration) = delegate minus o
    def -(o: Instant) = new Duration(o, delegate)
  }

  implicit class RichDateTime(val delegate: DateTime) extends AnyVal {
    def +(o: Duration) = delegate plus o
    def -(o: Duration) = delegate minus o
    def -(o: DateTime) = new Duration(o, delegate)
  }

  implicit class RichLocalTime(val delegate: LocalTime) extends AnyVal {
    def <(o: LocalTime) = delegate isBefore o
    def <=(o: LocalTime) = !(delegate isAfter o)
    def >(o: LocalTime) = delegate isAfter o
    def >=(o: LocalTime) = !(delegate isBefore o)
  }

//  implicit class RichDuration(val delegate: ReadableDuration) extends AnyVal {
//    def <(o: ReadableDuration) = delegate isShorterThan o
//    def <=(o: ReadableDuration) = !(delegate isLongerThan o)
//    def >(o: ReadableDuration) = delegate isLongerThan o
//    def >=(o: ReadableDuration) = !(delegate isShorterThan o)
//  }

  implicit object ReadableDurationOrdering extends Ordering[ReadableDuration] {
    def compare(x: ReadableDuration, y: ReadableDuration) =
      x.getMillis compare y.getMillis
  }

  implicit object DurationOrdering extends Ordering[Duration] {
    def compare(x: Duration, y: Duration) =
      x.getMillis compare y.getMillis
  }

  def sleep(d: Duration) {
    sleep(d.getMillis)
  }

  def sleep(millis: Long) = {
    val m = 1000000
    val until = System.nanoTime() + millis * m
    Thread.sleep(millis)
    @tailrec def extraSleep() {
      val remainingNanos = until - System.nanoTime()
      if (remainingNanos > 0) {
        extraSleepCount += 1
        Thread.sleep(remainingNanos / m, (remainingNanos % m).toInt)
        extraSleep()
      }
    }
    extraSleep()
  }
}
