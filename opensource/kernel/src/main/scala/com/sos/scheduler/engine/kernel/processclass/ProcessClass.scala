package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.cppproxy.Process_classC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

final class ProcessClass private(
  protected[this] val cppProxy: Process_classC,
  protected val subsystem: ProcessClassSubsystem)
extends FileBased {

  type Path = ProcessClassPath

  def stringToPath(o: String) = ProcessClassPath(o)

  def fileBasedType = FileBasedType.processClass

  def onCppProxyInvalidated() = {}
}


object ProcessClass {
  final class Type extends SisterType[ProcessClass, Process_classC] {
    def sister(proxy: Process_classC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new ProcessClass(proxy, injector.apply[ProcessClassSubsystem])
    }
  }
}
