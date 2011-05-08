package com.sos.scheduler.engine.playground.zschimmer


import com.sos.scheduler.engine.kernel.util.Time

object Threads {
    def untilInterrupted(f: => Unit) {
        try while (!Thread.interrupted) f
        catch { case x: InterruptedException => }
    }

    def untilInterruptedEvery(t: Time)(f: => Unit) {
        val timer = new IntervalTimer(t.getMillis)
        untilInterrupted {
            f
            Thread.sleep(timer.msUntilNextInterval())
        }
    }

    def interruptAndJoinThread(t: Thread) {
        t.interrupt()
        t.join()
    }
}
