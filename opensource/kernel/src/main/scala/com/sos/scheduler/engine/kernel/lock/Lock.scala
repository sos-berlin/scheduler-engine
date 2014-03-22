package com.sos.scheduler.engine.kernel.lock

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.kernel.cppproxy.LockC
import com.sos.scheduler.engine.kernel.filebased.FileBased

final class Lock private(
  protected[this] val cppProxy: LockC)
extends FileBased {

  type Path = LockPath

  def stringToPath(o: String) = LockPath(o)

  def fileBasedType = FileBasedType.lock

  def onCppProxyInvalidated() {}
}

@ForCpp
object Lock {
  final class Type extends SisterType[Lock, LockC] {
    def sister(proxy: LockC, context: Sister) = new Lock(proxy)
  }
}
