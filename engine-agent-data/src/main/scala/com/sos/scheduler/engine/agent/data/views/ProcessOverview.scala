package com.sos.scheduler.engine.agent.data.views

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits.InstantJsonFormat
import java.time.Instant
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class ProcessOverview(id: AgentProcessId, controllerAddress: String, startedAt: Instant)

object ProcessOverview {
  implicit val MyJsonFormat = jsonFormat3(apply)
}
