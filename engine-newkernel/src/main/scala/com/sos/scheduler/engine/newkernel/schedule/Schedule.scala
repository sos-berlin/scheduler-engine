package com.sos.scheduler.engine.newkernel.schedule

import org.joda.time.{Interval, Instant}

trait Schedule {
  def nextInstant(o: Instant): Option[Instant]

  def nextInterval(from: Instant): Option[Interval]
}

object Schedule {
  val eternalInterval = new Interval(new Instant(0), new Instant(Long.MaxValue))

  object Default extends Schedule {
    def nextInstant(o: Instant) = None

    def nextInterval(from: Instant) = Some(eternalInterval)
  }
}
