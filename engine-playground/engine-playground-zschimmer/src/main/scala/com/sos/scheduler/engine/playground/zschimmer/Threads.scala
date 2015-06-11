package com.sos.scheduler.engine.playground.zschimmer

import java.time.Duration

object Threads {

  def untilInterruptedEvery(duration: Duration)(f: => Unit): Unit = {
    val timer = new IntervalTimer(duration.toMillis)
    untilInterrupted {
      f
      Thread.sleep(timer.msUntilNextInterval())
    }
  }

  def untilInterrupted(f: => Unit): Unit = {
    try while (!Thread.interrupted) f
    catch {
      case x: InterruptedException =>
      case x: RuntimeException if x.getCause.isInstanceOf[InterruptedException] =>
    }
  }

  def interruptAndJoinThread(t: Thread): Unit = {
    t.interrupt()
    t.join()
  }
}
