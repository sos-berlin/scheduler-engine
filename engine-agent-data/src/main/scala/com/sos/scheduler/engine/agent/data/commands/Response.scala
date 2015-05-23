package com.sos.scheduler.engine.agent.data.commands

import spray.json._

/**
 * @author Joacim Zschimmer
 */
trait Response

object Response {
  implicit object MyJsonFormat extends RootJsonWriter[Response] {
    def write(response: Response) = response match {
      case o: FileOrderSourceContent ⇒ o.toJson
      case o ⇒ throw new UnsupportedOperationException(s"Class ${o.getClass.getName} is not serializable to JSON")
    }
  }
}
