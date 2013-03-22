package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.{PoppableCallQueue, CallQueue, TimedCall}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import SchedulerThreadCallQueue._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.common.scalautil.Logger

final class SchedulerThreadCallQueue(val delegate: PoppableCallQueue, cppProxy: SpoolerC, val thread: Thread) extends CallQueue {

  def close() {
    delegate.close()
  }

  def add(o: TimedCall[_]) = {
    logger debug s"Enqueue $o"
    delegate.add(o)
    try cppProxy.signal()
    catch { case e: CppProxyInvalidatedException => }   // Das passiert, wenn der TimedCall den Scheduler beendet und er beim signal() schon beendet ist.
  }

  def tryCancel(o: TimedCall[_]) = delegate.tryCancel(o)

  def nextTime = delegate.nextTime
}

object SchedulerThreadCallQueue {
  private val logger = Logger(classOf[SchedulerThreadCallQueue])
}