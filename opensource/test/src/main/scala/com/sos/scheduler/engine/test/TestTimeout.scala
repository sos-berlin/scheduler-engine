package com.sos.scheduler.engine.test

import java.util.concurrent.TimeUnit
import org.joda.time.Duration

final case class TestTimeout(duration: Duration) {
  def concurrentDuration =
    concurrent.duration.Duration(duration.getMillis, TimeUnit.MILLISECONDS)
}
