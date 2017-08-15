package com.sos.scheduler.engine.agent.data.commands

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class StartNonApiTask(
  meta: Option[StartTask.Meta],
  logon: Option[StartTask.KeyLogon])
extends StartTask

object StartNonApiTask {
  val SerialTypeName = "StartNonApiTask"
  implicit val MyJsonFormat = jsonFormat2(apply)
}
