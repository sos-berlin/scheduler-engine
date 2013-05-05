package com.sos.scheduler.engine.common.time

import org.joda.time.Duration.{millis, standardSeconds, standardDays}
import org.joda.time.{DateTime, LocalTime, Duration, Instant}

object ScalaJoda {
  implicit class DurationRichInt(val int: Int) extends AnyVal {
    final def ms = millis(int)
    final def s = standardSeconds(int)
    final def hours = standardSeconds(int)
    final def days = standardDays(int)
    final def *(o: Duration) = millis(int * o.getMillis)
  }

  implicit class DurationRichLong(val long: Long) extends AnyVal {
    final def ms = millis(long)
    final def s = standardSeconds(long)
    final def hours = standardSeconds(long)
    final def days = standardDays(long)
    final def *(o: Duration) = millis(long * o.getMillis)
  }

  implicit class RichInstant(val instant: Instant) extends AnyVal {
    def +(o: Duration) = instant plus o
    def -(o: Duration) = instant minus o
  }

  implicit class RichDateTime(val dateTime: DateTime) extends AnyVal {
    def +(o: Duration) = dateTime plus o
    def -(o: Duration) = dateTime minus o
  }

  implicit class RichLocalTime(val delegate: LocalTime) extends AnyVal {
    def <(o: LocalTime) = delegate isBefore o
    def <=(o: LocalTime) = !(delegate isAfter o)
    def >(o: LocalTime) = delegate isAfter o
    def >=(o: LocalTime) = !(delegate isBefore o)
  }

  implicit object DurationOrdering extends Ordering[Duration]{
    def compare(x: Duration, y: Duration) =
      x.getMillis compare y.getMillis
  }

  def sleep(d: Duration) =
    Thread.sleep(d.getMillis)
}
