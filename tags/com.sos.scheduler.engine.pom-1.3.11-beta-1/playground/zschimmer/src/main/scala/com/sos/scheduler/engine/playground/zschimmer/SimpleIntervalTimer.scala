package com.sos.scheduler.engine.playground.zschimmer

import java.lang.Math.max


class SimpleIntervalTimer(val intervalMs: Long, val startTime: Long) {
    def numberOfMs(t: Long) = (t - startTime) / intervalMs
    def nextNumberOfMs(t: Long) = numberOfMs(t) + 1
    def nearestNumberOfMs(t: Long) = numberOfMs(t + max(1, intervalMs - 1))

    def nextMs(t: Long): Long = startTime + nextNumberOfMs(t) * intervalMs
    def msUntilNextInterval(t: Long): Long = nextMs(t) - t

//    def nearestMs(t: Long): Long = startTime + nearestNumberOfMs(t) * intervalMs
}
