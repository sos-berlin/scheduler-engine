package com.sos.scheduler.engine.common.time

import org.joda.time.Instant

object CppTime {
  private val EternalCppMillis = Int.MaxValue * 1000L

  def cppTimeToInstantOption(millis: Long): Option[Instant] =
    millis match {
      case EternalCppMillis => None
      case o => Some(new Instant(millis))
    }
}
