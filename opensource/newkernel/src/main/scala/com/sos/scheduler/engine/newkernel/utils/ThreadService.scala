package com.sos.scheduler.engine.newkernel.utils

import java.util.concurrent.atomic.AtomicReference

final class ThreadService(runnable: Runnable) extends Service {
  private val threadRef = new AtomicReference[Thread]

  def close() {
    for (t <- threadOption) {
      stop()
      t.join()
      threadRef.set(null)
    }
  }

  protected def onStart() {
    def throwAlreadyStarted() = throw new IllegalStateException(s"$toString has been started already")
    if (threadRef.get != null) throwAlreadyStarted()
    val t = new Thread(runnable, toString)
    val ok = threadRef.compareAndSet(null, t)
    if (!ok) throwAlreadyStarted()
    t.start()
  }


  protected def onStop() {
    for (t <- threadOption)
      t.interrupt()
  }

  def thread =
    threadOption getOrElse { throw new IllegalStateException(s"$toString is not running") }

  def threadOption =
    Option(threadRef.get)

  override def toString =
    s"Service '$runnable'"
}
