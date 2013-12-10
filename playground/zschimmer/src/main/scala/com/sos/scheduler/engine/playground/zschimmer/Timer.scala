package com.sos.scheduler.engine.playground.zschimmer

import java.lang.System.currentTimeMillis
import org.joda.time.Duration

final class Timer(duration: Duration) {
  val startTime = now
  val endTime = startTime + duration.getMillis

  def isElapsed =
    now >= endTime

  def elapsedMs =
    now - startTime

  private def now =
    currentTimeMillis()

  override def toString =
    (elapsedMs / 1000.0) + "s"
}

//object Timer {
//    @deprecated("") def time[A](timeout: Time)(f: => A) = {
//        val start = System.currentTimeMillis
//        val result = f
//        new {
//            def whenTimedOut(f: Time => Unit) {
//                val now = System.currentTimeMillis
//                val elapsed = now - start
//                if (elapsed > timeout.getMillis)  f(Time.ofMillis(elapsed))
//                result
//            }
//        }
//    }
//}
