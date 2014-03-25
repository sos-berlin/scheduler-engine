package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedType}
import spray.json.DefaultJsonProtocol._

final case class SimpleSubsystemOverview(
  fileBasedType: FileBasedType,
  count: Int,
  fileBasedStateCounts: Map[FileBasedState, Int])
extends FileBasedSubsystemOverview


object SimpleSubsystemOverview {
  private implicit val fileBasedTypeJsonFormat = FileBasedType.MyJsonFormat
  private implicit val fileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat3(apply)
}
