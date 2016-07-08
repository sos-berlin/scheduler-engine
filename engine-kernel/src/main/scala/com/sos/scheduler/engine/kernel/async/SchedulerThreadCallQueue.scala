package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.{CallQueue, PoppableCallQueue, TimedCall}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue._
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC

final class SchedulerThreadCallQueue(val delegate: PoppableCallQueue, spoolerC: SpoolerC, val cppThread: Thread) extends CallQueue {

  def close(): Unit = {
    delegate.close()
  }

  def add[A](o: TimedCall[A]) = {
    delegate.add(o)
    try spoolerC.signal()
    catch {
      // Das passiert, wenn der TimedCall den Scheduler beendet und er beim signal() schon beendet ist.
      // CppProxyInvalidatedException or - under Unix - ERRNO-9 Bad file descriptor due to closed Internal_signaling_socket.
      case e: CppProxyInvalidatedException ⇒ logger.debug(e.toString)
    }
  }

  def tryCancel[A](o: TimedCall[A]) =
    delegate.tryCancel(o)

  def nextTime =
    delegate.nextTime
}

object SchedulerThreadCallQueue {
  private val logger = Logger(getClass)
}
