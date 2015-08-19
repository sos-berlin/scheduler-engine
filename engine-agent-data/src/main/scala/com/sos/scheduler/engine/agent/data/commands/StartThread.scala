package com.sos.scheduler.engine.agent.data.commands

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
case object StartThread extends StartProcess {
  val SerialTypeName = "StartThread"
  implicit val MyJsonFormat = jsonFormat0(() ⇒ StartThread)
}
