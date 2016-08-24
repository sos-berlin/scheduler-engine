package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.scheduler.engine.base.sprayjson.SprayJson.implicits._
import com.sos.scheduler.engine.data.filebased.{FileBasedDetails, FileBasedState}
import java.nio.file.Path
import java.time.Instant
import spray.json.DefaultJsonProtocol._

final case class SimpleFileBasedDetails(
  overview: SimpleFileBasedOverview,
  file: Option[Path],
  fileModifiedAt: Option[Instant],
  sourceXml: Option[String])
extends FileBasedDetails

object SimpleFileBasedDetails {
  private implicit val FileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat4(apply)
}
