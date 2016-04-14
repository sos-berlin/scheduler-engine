package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.base.generic.JavaJsonFormats._
import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.scheduler.engine.data.filebased.{FileBasedDetails, FileBasedState, TypedPath}
import java.io.File
import java.time.Instant
import spray.json.DefaultJsonProtocol._

final case class SimpleFileBasedDetails(
  path: TypedPath,
  fileBasedState: FileBasedState,
  file: Option[File],
  fileModificationInstant: Option[Instant],
  sourceXml: Option[String])
extends FileBasedDetails


object SimpleFileBasedDetails {
  private implicit val FileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat5(apply)
}
