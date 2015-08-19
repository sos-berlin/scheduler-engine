package com.sos.scheduler.engine.agent.data.commands

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class StartApiTask(javaOptions: String, javaClasspath: String)
extends StartTask

object StartApiTask {
  val SerialTypeName = "StartApiTask"
  implicit val MyJsonFormat = jsonFormat2(apply)
}
