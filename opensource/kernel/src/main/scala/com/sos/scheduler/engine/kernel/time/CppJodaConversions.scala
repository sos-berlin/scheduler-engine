package com.sos.scheduler.engine.kernel.time

import com.sos.scheduler.engine.data.configuration.SchedulerDataConstants.eternalCppMillis
import org.joda.time.Instant

object CppJodaConversions {

  def eternalCppMillisToNoneInstant(millis: Long): Option[Instant] = {
    require(millis > 0, s"Timestamp from C++ is negative: $millis")
    Some(millis) filter { _ != eternalCppMillis } map { o => new Instant(o) }
  }

  def zeroCppMillisToNoneInstant(millis: Long): Option[Instant] =
    Some(millis) filter { _ != 0 } map { o => new Instant(o) }
}
