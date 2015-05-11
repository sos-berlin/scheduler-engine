package com.sos.scheduler.engine.agent.data

import com.sos.scheduler.engine.agent.data.FileOrderSourceContent._
import scala.collection.immutable
import spray.json.DefaultJsonProtocol._

final case class FileOrderSourceContent(files: immutable.Seq[Entry])

object FileOrderSourceContent {
  final case class Entry(path: String, lastModifiedTime: Long)

  object Entry {
    implicit val MyJsonFormat = jsonFormat2(apply)
  }

  implicit val MyJsonFormat = jsonFormat1(apply)
}
