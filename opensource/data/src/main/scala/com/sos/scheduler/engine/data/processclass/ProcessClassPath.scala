package com.sos.scheduler.engine.data.processclass

import com.sos.scheduler.engine.data.filebased.{FileBasedType, TypedPath}

final case class ProcessClassPath(string: String) extends TypedPath{
  override def fileBasedType = FileBasedType.processClass
}
