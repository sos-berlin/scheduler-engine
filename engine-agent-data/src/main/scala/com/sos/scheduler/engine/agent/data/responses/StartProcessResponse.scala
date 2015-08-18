package com.sos.scheduler.engine.agent.data.responses

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.tunnel.data.TunnelToken
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class StartProcessResponse(
  processId: AgentProcessId,
  tunnelToken: TunnelToken)
extends Response

object StartProcessResponse {
  implicit val MyJsonFormat = jsonFormat2(apply)
}
