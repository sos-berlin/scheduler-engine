package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.{PoppableCallQueue, CallQueue, TimedCall}
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC

final class SchedulerThreadCallQueue(val delegate: PoppableCallQueue, spoolerC: SpoolerC, val cppThread: Thread) extends CallQueue {

  def close() {
    delegate.close()
  }

  def add(o: TimedCall[_]) = {
    delegate.add(o)
    try spoolerC.signal()
    catch { case e: CppProxyInvalidatedException => }   // Das passiert, wenn der TimedCall den Scheduler beendet und er beim signal() schon beendet ist.
  }

  def tryCancel(o: TimedCall[_]) =
    delegate.tryCancel(o)

  def nextTime =
    delegate.nextTime
}
