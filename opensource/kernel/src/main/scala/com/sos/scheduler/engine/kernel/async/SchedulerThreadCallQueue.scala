package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.{PoppableCallQueue, CallQueue, TimedCall}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC

class SchedulerThreadCallQueue(val delegate: PoppableCallQueue, cppProxy: SpoolerC, val thread: Thread) extends CallQueue {
  def add(o: TimedCall[_]) = {
    delegate.add(o)
    wakeCppScheduler()
  }

  def tryRemove(o: TimedCall[_]) = delegate.tryRemove(o)

  def nextTime = delegate.nextTime

  private def wakeCppScheduler() {
    cppProxy.signal("TimedCall")
  }
}
