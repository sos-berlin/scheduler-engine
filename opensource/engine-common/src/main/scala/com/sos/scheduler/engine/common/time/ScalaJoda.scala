package com.sos.scheduler.engine.common.time

import org.joda.time.{Instant, Duration}

object ScalaJoda {
  implicit class DurationRichInt(val int: Int) extends AnyVal {
    final def ms = Duration.millis(int)
    final def s = Duration.standardSeconds(int)
    final def hours = Duration.standardSeconds(int)
    final def days = Duration.standardDays(int)
    final def *(o: Duration) = Duration.millis(4 * o.getMillis)
  }

  implicit class RichInstant(val instant: Instant) extends AnyVal {
    def +(o: Duration) = instant plus o
    def -(o: Duration) = instant minus o
  }

  def sleep(d: Duration) =
    Thread.sleep(d.getMillis)
}
