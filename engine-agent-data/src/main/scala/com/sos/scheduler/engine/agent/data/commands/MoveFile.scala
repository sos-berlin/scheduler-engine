package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.responses.EmptyResponse
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class MoveFile(path: String, newPath: String) extends Command {
  type Response = EmptyResponse.type
}

object MoveFile {
  val SerialTypeName = "MoveFile"
  implicit val MyJsonFormat = jsonFormat2(apply)
}
