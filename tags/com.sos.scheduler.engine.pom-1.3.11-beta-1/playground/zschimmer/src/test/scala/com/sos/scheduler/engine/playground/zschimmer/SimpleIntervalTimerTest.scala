package com.sos.scheduler.engine.playground.zschimmer

import org.junit._


class SimpleIntervalTimerTest {
    val startTime = 1000000
    val interval = 100
    val timer = new SimpleIntervalTimer(intervalMs=interval, startTime=startTime)
    
    @Test def testNumberOfMs() {
        assert(timer.numberOfMs(startTime) == 0)
        assert(timer.numberOfMs(startTime+1) == 0)
        assert(timer.numberOfMs(startTime+interval-1) == 0)
        assert(timer.numberOfMs(startTime+interval) == 1)
        assert(timer.numberOfMs(startTime+2*interval) == 2)
    }

    @Test def testNextNumberOfMs() {
        assert(timer.nextNumberOfMs(startTime) == 1)
        assert(timer.nextNumberOfMs(startTime+1) == 1)
        assert(timer.nextNumberOfMs(startTime+interval-1) == 1)
        assert(timer.nextNumberOfMs(startTime+interval) == 2)
        assert(timer.nextNumberOfMs(startTime+2*interval) == 3)
    }

    @Test def testMsUntilNextInterval() {
        assert(timer.msUntilNextInterval(startTime) == interval)
        assert(timer.msUntilNextInterval(startTime+1) == interval - 1)
        assert(timer.msUntilNextInterval(startTime+interval-1) == 1)
        assert(timer.msUntilNextInterval(startTime+interval) == interval)
        assert(timer.msUntilNextInterval(startTime+2*interval) == interval)
    }

//    def nextNumberOfMs(t: Long) = numberOfMs(t) + 1
//    def nearestNumberOfMs(t: Long) = numberOfMs(t + max(1, intervalMs - 1))
//
//    def nextMs(t: Long): Long = startTime + nextNumberOfMs(t) * intervalMs
//    def msUntilNextInterval(t: Long): Long = nextMs(t) - t
}
