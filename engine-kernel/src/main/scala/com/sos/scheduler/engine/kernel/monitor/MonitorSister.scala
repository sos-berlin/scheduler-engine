package com.sos.scheduler.engine.kernel.monitor

import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.monitor.MonitorPath
import com.sos.scheduler.engine.kernel.cppproxy.MonitorC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

final class MonitorSister(
  protected[this] val cppProxy: MonitorC,
  protected[kernel] val subsystem: MonitorSubsystem)
extends FileBased
with Sister
{
  protected type Self = MonitorSister
  type ThisPath = MonitorPath

  def stringToPath(o: String) = MonitorPath(o)

  def onCppProxyInvalidated() = {}
}

object MonitorSister
{
  private[kernel] object Type extends SisterType[MonitorSister, MonitorC] {
    def sister(proxy: MonitorC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new MonitorSister(proxy, injector.instance[MonitorSubsystem])
    }
  }
}
