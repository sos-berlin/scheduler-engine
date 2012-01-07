package com.sos.scheduler.engine.playground.zschimmer

import com.sos.scheduler.engine.kernel.util.Time

object Threads {
  def untilInterruptedEvery(t: Time)(f: => Unit) {
    val timer = new IntervalTimer(t.getMillis)
    untilInterrupted {
      f
      Thread.sleep(timer.msUntilNextInterval())
    }
  }

    def untilInterrupted(f: => Unit) {
        try while (!Thread.interrupted) f
        catch {
          case x: InterruptedException =>
          case x: RuntimeException if x.getCause.isInstanceOf[InterruptedException] =>
        }
    }

    def interruptAndJoinThread(t: Thread) {
        t.interrupt()
        t.join()
    }
}
