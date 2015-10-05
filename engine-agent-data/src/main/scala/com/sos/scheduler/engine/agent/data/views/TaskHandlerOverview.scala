package com.sos.scheduler.engine.agent.data.views

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class TaskHandlerOverview(
  isTerminating: Boolean,
  currentTaskCount: Int,
  totalTaskCount: Int)

object TaskHandlerOverview {
  implicit val MyJsonFormat = jsonFormat3(apply)
}
