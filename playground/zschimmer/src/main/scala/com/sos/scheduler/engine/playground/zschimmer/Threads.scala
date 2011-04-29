package com.sos.scheduler.engine.playground.zschimmer


object Threads {
    def untilInterrupted(f: => Unit) {
        try while (!Thread.interrupted) f
        catch { case x: InterruptedException => }
    }

    def interruptAndJoinThread(t: Thread) {
        t.interrupt()
        t.join()
    }
}
