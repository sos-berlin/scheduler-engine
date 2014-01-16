package com.sos.scheduler.engine.kernel.time

import com.sos.scheduler.engine.data.configuration.SchedulerDataConstants.eternalMillis
import org.joda.time.Instant

object CppJodaConversions {

  def eternalMillisToNone(millis: Long): Option[Instant] = {
    require(millis > 0, s"Timestamp from C++ is negative: $millis")
    Some(millis) filter { _ != eternalMillis } map { o => new Instant(o) }
  }

  def zeroMillisToNone(millis: Long): Option[Instant] =
    Some(millis) filter { _ != 0 } map { o => new Instant(o) }
}
