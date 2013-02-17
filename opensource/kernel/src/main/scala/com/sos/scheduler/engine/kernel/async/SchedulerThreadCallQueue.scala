package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.{PoppableCallQueue, CallQueue, TimedCall}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import org.slf4j.LoggerFactory
import SchedulerThreadCallQueue._

class SchedulerThreadCallQueue(val delegate: PoppableCallQueue, cppProxy: SpoolerC, val thread: Thread) extends CallQueue {
  def add(o: TimedCall[_]) = {
    if (logger.isDebugEnabled) logger.debug(s"Enqueue $o")
    delegate.add(o)
    wakeCppScheduler()
  }

  def tryRemove(o: TimedCall[_]) = delegate.tryRemove(o)

  def nextTime = delegate.nextTime

  private def wakeCppScheduler() {
    cppProxy.signal("TimedCall")
  }
}

object SchedulerThreadCallQueue {
  private val logger = LoggerFactory.getLogger(classOf[SchedulerThreadCallQueue])
}