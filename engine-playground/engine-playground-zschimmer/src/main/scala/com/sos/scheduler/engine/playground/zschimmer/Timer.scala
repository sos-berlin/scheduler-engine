package com.sos.scheduler.engine.playground.zschimmer

import java.lang.System.currentTimeMillis
import java.time.Duration

final class Timer(duration: Duration) {
  val startTime = now
  val endTime = startTime + duration.toMillis

  def isElapsed =
    now >= endTime

  def elapsedMs =
    now - startTime

  private def now =
    currentTimeMillis()

  override def toString =
    (elapsedMs / 1000.0) + "s"
}
