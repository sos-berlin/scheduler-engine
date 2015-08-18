package com.sos.scheduler.engine.agent.data.commands

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class StartSeparateProcess(javaOptions: String, javaClasspath: String)
extends StartProcess

object StartSeparateProcess {
  val SerialTypeName = "StartSeparateProcess"
  implicit val MyJsonFormat = jsonFormat2(apply)
}
