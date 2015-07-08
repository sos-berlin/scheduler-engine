package com.sos.scheduler.engine.agent.data.commands

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class StartSeparateProcess(controllerAddressOption: Option[String], javaOptions: String, javaClasspath: String)
extends StartProcess

object StartSeparateProcess {
  val SerialTypeName = "StartSeparateProcess"
  implicit val MyJsonFormat = jsonFormat3(apply)
}
