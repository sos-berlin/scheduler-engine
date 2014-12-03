package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.base.JavaJsonFormats._
import com.sos.scheduler.engine.data.base.JodaJsonFormats._
import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedDetails, TypedPath}
import java.io.File
import org.joda.time.Instant
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
