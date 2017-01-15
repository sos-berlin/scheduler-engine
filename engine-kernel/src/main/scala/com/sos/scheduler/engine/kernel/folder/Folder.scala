package com.sos.scheduler.engine.kernel.folder

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.kernel.cppproxy.FolderC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

private[kernel] final class Folder(
  protected[this] val cppProxy: FolderC,
  protected[kernel] val subsystem: FolderSubsystem)
extends FileBased {

  protected type Self = Folder
  type ThisPath = FolderPath

  def stringToPath(o: String) = FolderPath(o)

  def onCppProxyInvalidated(): Unit = {}
}


object Folder {
  final class Type extends SisterType[Folder, FolderC] {
    def sister(proxy: FolderC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Folder(proxy, injector.instance[FolderSubsystem])
    }
  }
}
