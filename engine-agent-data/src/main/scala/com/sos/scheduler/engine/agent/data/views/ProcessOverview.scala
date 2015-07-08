package com.sos.scheduler.engine.agent.data.views

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits.InstantJsonFormat
import com.sos.scheduler.engine.tunnel.data.TunnelId
import java.time.Instant
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class ProcessOverview(
  id: AgentProcessId,
  tunnelId: Option[TunnelId],
  controllerAddress: String,
  startedAt: Instant)

object ProcessOverview {
  implicit val MyJsonFormat = jsonFormat4(apply)
}
