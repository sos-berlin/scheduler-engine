package com.sos.scheduler.engine.playground.zschimmer

import com.sos.scheduler.engine.kernel.util.Time
import java.util.Date


class Timer(timeout: Time) {
    val startTime = now
    val endTime = startTime + timeout.getMillis
    def isElapsed = now >= endTime
    def elapsedMs = now - startTime
    private def now = new Date().getTime
    override def toString = (elapsedMs / 1000.0) + "s"
}


object Timer {
    @deprecated("") def time[A](timeout: Time)(f: => A) = {
        val start = System.currentTimeMillis
        val result = f
        new {
            def whenTimedOut(f: Time => Unit) {
                val now = System.currentTimeMillis
                val elapsed = now - start
                if (elapsed > timeout.getMillis)  f(Time.ofMillis(elapsed))
                result
            }
        }
    }
}

//                        Timer.time(conf.timeout) {
//                            scheduler.callCppAndDoNothing()
//                        } whenTimedOut { t => logger.warn("Scheduler response time was " + t) }
