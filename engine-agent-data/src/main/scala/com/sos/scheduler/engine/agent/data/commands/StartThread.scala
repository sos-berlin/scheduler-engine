package com.sos.scheduler.engine.agent.data.commands

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class StartThread(controllerAddressOption: Option[String])
extends StartProcess

object StartThread {
  val SerialTypeName = "StartThread"
  implicit val MyJsonFormat = jsonFormat1(apply)
}
