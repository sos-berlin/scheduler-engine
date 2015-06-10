package com.sos.scheduler.engine.test

import java.util.concurrent.TimeUnit
import java.time.Duration
import scala.language.implicitConversions

final case class ImplicitTimeout(duration: Duration) {

  def concurrentDuration = concurrent.duration.Duration(duration.toMillis, TimeUnit.MILLISECONDS)
}

object ImplicitTimeout {
  implicit def durationToImplicitDuration(duration: Duration): ImplicitTimeout = new ImplicitTimeout(duration)
}
