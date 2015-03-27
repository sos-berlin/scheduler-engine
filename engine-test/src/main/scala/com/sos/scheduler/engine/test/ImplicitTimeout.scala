package com.sos.scheduler.engine.test

import java.util.concurrent.TimeUnit
import org.joda.time.Duration
import scala.language.implicitConversions

final case class ImplicitTimeout(duration: Duration) {

  def concurrentDuration = concurrent.duration.Duration(duration.getMillis, TimeUnit.MILLISECONDS)
}

object ImplicitTimeout {
  implicit def durationToImplicitDuration(duration: Duration): ImplicitTimeout = new ImplicitTimeout(duration)
}
