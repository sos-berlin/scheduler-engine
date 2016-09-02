package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.scheduler.engine.base.sprayjson.SprayJson.JsonFormats._
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, FileBasedState}
import java.nio.file.Path
import java.time.Instant
import spray.json.DefaultJsonProtocol._

final case class SimpleFileBasedDetailed(
  overview: SimpleFileBasedOverview,
  file: Option[Path],
  fileModifiedAt: Option[Instant],
  sourceXml: Option[String])
extends FileBasedDetailed

object SimpleFileBasedDetailed {
  private implicit val FileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat4(apply)
}
