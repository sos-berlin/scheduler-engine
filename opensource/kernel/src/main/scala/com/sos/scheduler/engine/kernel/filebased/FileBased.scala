package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.{FileBasedType, TypedPath}
import com.sos.scheduler.engine.eventbus.EventSource

@ForCpp abstract class FileBased
extends Sister
with EventSource {

  type Path <: TypedPath

  /** Jedes Exemplar hat seine eigene UUID. */
  final val uuid = java.util.UUID.randomUUID

  @Deprecated final def getUuid = uuid
  @Deprecated final def getFileBasedType = fileBasedType
  @Deprecated final def getPath = path
  @Deprecated final def getTypedPath = path

  def fileBasedType: FileBasedType

  def path: Path

  override def toString = path.toString
}
