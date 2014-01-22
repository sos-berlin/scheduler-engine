package com.sos.scheduler.engine.kernel.folder

import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.AbsolutePath
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.TypedPath
import com.sos.scheduler.engine.eventbus.EventSource

@ForCpp abstract class FileBased
extends Sister
with EventSource {

  /** Jedes Exemplar hat seine eigene UUID. */
  final val uuid = java.util.UUID.randomUUID

  @Deprecated final def getUuid = uuid
  @Deprecated final def getFileBasedType = fileBasedType
  @Deprecated final def getPath = path
  @Deprecated final def getTypedPath = typedPath

  def fileBasedType: FileBasedType

  def path: AbsolutePath


  final def typedPath: TypedPath =
    fileBasedType.typedPath(path.string)

  override def toString =
    s"$fileBasedType $path"
}

