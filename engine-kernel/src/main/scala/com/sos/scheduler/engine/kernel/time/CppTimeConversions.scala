package com.sos.scheduler.engine.kernel.time

import com.sos.scheduler.engine.data.configuration.SchedulerDataConstants.{EternalCppMillis, NeverCppMillis}
import java.time.{Duration, Instant}

object CppTimeConversions {

  def eternalCppMillisToNoneInstant(millis: Long): Option[Instant] = {
    require(millis > 0, s"Timestamp from C++ is negative: $millis")
    if (millis == EternalCppMillis)
      None
    else
      Some(Instant.ofEpochMilli(millis))
  }

  def zeroCppMillisToNoneInstant(millis: Long): Option[Instant] =
    if (millis == 0)
      None
    else
      Some(Instant.ofEpochMilli(millis))

  def neverCppMillisToNoneDuration(millis: Long): Option[Duration] =
    if (millis == NeverCppMillis)
      None
    else
      Some(Duration.ofMillis(millis))
}
