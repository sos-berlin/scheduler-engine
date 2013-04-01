package com.sos.scheduler.engine.common.time

import org.joda.time.Duration.{millis, standardSeconds, standardDays}
import org.joda.time.{LocalTime, Duration, Instant}

object ScalaJoda {
  implicit class DurationRichInt(val int: Int) extends AnyVal {
    final def ms = millis(int)
    final def s = standardSeconds(int)
    final def hours = standardSeconds(int)
    final def days = standardDays(int)
    final def *(o: Duration) = millis(int * o.getMillis)
  }

  implicit class RichInstant(val instant: Instant) extends AnyVal {
    def +(o: Duration) = instant plus o
    def -(o: Duration) = instant minus o
  }

  implicit class RichLocalTime(val delegate: LocalTime) extends AnyVal {
    def <(o: LocalTime) = delegate isBefore o
    def <=(o: LocalTime) = !(delegate isAfter o)
    def >(o: LocalTime) = delegate isAfter o
    def >=(o: LocalTime) = !(delegate isBefore o)
  }

  def sleep(d: Duration) =
    Thread.sleep(d.getMillis)
}
