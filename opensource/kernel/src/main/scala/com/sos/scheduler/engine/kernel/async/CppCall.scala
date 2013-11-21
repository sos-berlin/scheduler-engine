package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.TimedCall
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.Timed_callC

@ForCpp
final class CppCall(cppProxy: Timed_callC)
extends TimedCall[Unit] with Sister {

  lazy val epochMillis =
    cppProxy.at_millis()

  def value =
    cppProxy.value()

  def value_=(o: AnyRef) {
    cppProxy.set_value(o)
  }

  def call() =
    cppProxy.call()

  def onCppProxyInvalidated() {}

  override def toStringPrefix =
    cppProxy.obj_name
}