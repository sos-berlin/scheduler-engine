package com.sos.scheduler.engine.kernel.lock

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.kernel.cppproxy.LockC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

private[kernel] final class Lock private(
  protected[this] val cppProxy: LockC,
  protected[kernel] val subsystem: LockSubsystem)
extends FileBased {

  protected type Self = Lock
  type ThisPath = LockPath

  def stringToPath(o: String) = LockPath(o)

  def onCppProxyInvalidated(): Unit = {}
}

object Lock {
  object Type extends SisterType[Lock, LockC] {
    def sister(proxy: LockC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Lock(proxy, injector.instance[LockSubsystem])
    }
  }
}
