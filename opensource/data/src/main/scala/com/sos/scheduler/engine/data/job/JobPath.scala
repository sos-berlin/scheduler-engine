package com.sos.scheduler.engine.data.job

import com.fasterxml.jackson.annotation.JsonCreator
import com.sos.scheduler.engine.data.filebased.{AbsolutePath, FileBasedType, TypedPath}

final case class JobPath(string: String)
extends TypedPath {

  assertIsAbsolute()

  def fileBasedType = FileBasedType.job
}


object JobPath {
  @JsonCreator def of(absolutePath: String): JobPath =
    new JobPath(absolutePath)

  def makeAbsolute(path: String) =
    new JobPath(AbsolutePath.makeAbsolute(path))
}
