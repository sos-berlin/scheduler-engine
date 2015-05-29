package com.sos.scheduler.engine.agent.data.commands

import scala.collection.immutable
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class FileOrderSourceContent(files: immutable.Seq[FileOrderSourceContent.Entry])
extends Response {
  def isEmpty = files.isEmpty
}

object FileOrderSourceContent {
  final case class Entry(path: String, lastModifiedTime: Long)

  object Entry {
    implicit val MyJsonFormat = jsonFormat2(apply)
  }

  implicit val MyJsonFormat = jsonFormat1(apply)
}
