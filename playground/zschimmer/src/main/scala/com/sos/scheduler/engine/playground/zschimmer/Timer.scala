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
