package com.sos.scheduler.engine.kernel.folder

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.kernel.filebased.FileBased

final class Folder(val path: FolderPath) extends FileBased {

  type Path = FolderPath

  def fileBasedType = FileBasedType.folder

  def onCppProxyInvalidated() {}
}
