package com.sos.scheduler.engine.agent.data

import scala.collection.immutable
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class RequestFileOrderSourceContent(
  directory: String,
  regex: String,
  durationMillis: Long,
  knownFiles: immutable.Set[String])

object RequestFileOrderSourceContent {
  implicit val MyJsonFormat = jsonFormat4(apply)
}
