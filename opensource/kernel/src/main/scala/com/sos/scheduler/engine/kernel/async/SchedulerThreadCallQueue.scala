package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.{PoppableCallQueue, CallQueue, TimedCall}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import org.slf4j.LoggerFactory
import SchedulerThreadCallQueue._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException

class SchedulerThreadCallQueue(val delegate: PoppableCallQueue, cppProxy: SpoolerC, val thread: Thread) extends CallQueue {
  def add(o: TimedCall[_]) = {
    if (logger.isDebugEnabled) logger.debug(s"Enqueue $o")
    delegate.add(o)
    try cppProxy.signal("TimedCall")
    catch { case e: CppProxyInvalidatedException => }   // Das passiert, wenn der TimedCall den Scheduler beendet und er beim signal() schon beendet ist.
  }

  def tryRemove(o: TimedCall[_]) = delegate.tryRemove(o)

  def nextTime = delegate.nextTime

}

object SchedulerThreadCallQueue {
  private val logger = LoggerFactory.getLogger(classOf[SchedulerThreadCallQueue])
}