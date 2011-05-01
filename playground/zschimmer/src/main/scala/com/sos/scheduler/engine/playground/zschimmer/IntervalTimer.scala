package com.sos.scheduler.engine.playground.zschimmer


class IntervalTimer(override val intervalMs: Long, override val startTime: Long = System.currentTimeMillis)
extends SimpleIntervalTimer(intervalMs, startTime) {
    def number() = numberOfMs(System.currentTimeMillis)
    def nextMs(): Long = nextMs(System.currentTimeMillis)
    def msUntilNextInterval(): Long = msUntilNextInterval(System.currentTimeMillis)
}
