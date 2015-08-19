package com.sos.scheduler.engine.agent.data.commands

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
case object StartNonApiTask extends StartTask {
  val SerialTypeName = "StartNonApiTask"
  implicit val MyJsonFormat = jsonFormat0(() ⇒ StartNonApiTask)
}
