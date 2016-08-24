package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.{FileBasedOverview, FileBasedState, TypedPath}
import spray.json.DefaultJsonProtocol._

final case class SimpleFileBasedOverview(
  path: TypedPath,
  fileBasedState: FileBasedState)
extends FileBasedOverview

object SimpleFileBasedOverview {
  private implicit val FileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat2(apply)
}
