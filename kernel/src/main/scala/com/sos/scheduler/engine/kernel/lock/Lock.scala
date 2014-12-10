package com.sos.scheduler.engine.kernel.lock

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.kernel.cppproxy.LockC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

final class Lock private(protected[this] val cppProxy: LockC, protected val subsystem: LockSubsystem)
extends FileBased {

  type Path = LockPath

  def stringToPath(o: String) = LockPath(o)

  def fileBasedType = FileBasedType.lock

  def onCppProxyInvalidated(): Unit = {}
}

object Lock {
  final class Type extends SisterType[Lock, LockC] {
    def sister(proxy: LockC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Lock(proxy, injector.apply[LockSubsystem])
    }
  }
}
