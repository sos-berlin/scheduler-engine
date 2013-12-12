package com.sos.scheduler.engine.data.folder

import com.fasterxml.jackson.annotation.JsonCreator

final case class JobPath(string: String)
extends TypedPath {

  assertIsAbsolute()

  def typ =
    FileBasedType.job
}


object JobPath {
  @JsonCreator def of(absolutePath: String): JobPath =
    new JobPath(absolutePath)

  def makeAbsolute(path: String) =
    new JobPath(AbsolutePath.makeAbsolute(path))
}
