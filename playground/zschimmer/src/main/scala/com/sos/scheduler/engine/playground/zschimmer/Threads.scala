package com.sos.scheduler.engine.playground.zschimmer

import org.joda.time.Duration

object Threads {

  def untilInterruptedEvery(duration: Duration)(f: => Unit) {
    val timer = new IntervalTimer(duration.getMillis)
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
