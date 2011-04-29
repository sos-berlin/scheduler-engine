package com.sos.scheduler.engine.playground.zschimmer


@deprecated("") class IntervalTimer(val intervalMs: Long) {
    val startTime = System.currentTimeMillis

    def number() = numberOfTimeMs(System.currentTimeMillis)
    def nextTimeMs(): Long = nextTimeMs(System.currentTimeMillis)

    def numberOfTimeMs(t: Long) = (t - startTime) / intervalMs

    def nextTimeMs(t: Long): Long = startTime + number() * intervalMs
}
