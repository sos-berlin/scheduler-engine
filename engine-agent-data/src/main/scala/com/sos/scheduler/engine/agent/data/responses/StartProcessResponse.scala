package com.sos.scheduler.engine.agent.data.responses

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.tunnel.data.TunnelToken

/**
 * @author Joacim Zschimmer
 */
final case class StartProcessResponse(
  processId: AgentProcessId,
  tunnelIdWithPasswordOption: Option[TunnelToken])
extends Response
