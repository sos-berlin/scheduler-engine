package com.sos.scheduler.engine.kernel.folder

import com.sos.scheduler.engine.cplusplus.runtime.{SisterType, Sister}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.kernel.cppproxy.FolderC
import com.sos.scheduler.engine.kernel.filebased.FileBased

final class Folder(protected[this] val cppProxy: FolderC)
extends FileBased {

  type Path = FolderPath

  def stringToPath(o: String) = FolderPath(o)

  def fileBasedType = FileBasedType.folder

  def onCppProxyInvalidated() {}
}


object Folder {
  final class Type extends SisterType[Folder, FolderC] {
    def sister(proxy: FolderC, context: Sister) = {
      assert(context == null)
      new Folder(proxy)
    }
  }
}
