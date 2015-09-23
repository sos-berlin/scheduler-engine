package com.sos.scheduler.engine.agent.data.views

import scala.collection.immutable
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class TaskHandlerView(
  isTerminating: Boolean,
  currentTaskCount: Int,
  totalTaskCount: Int,
  tasks: immutable.Seq[TaskOverview])

object TaskHandlerView {
  implicit val MyJsonFormat = jsonFormat4(apply)
}
