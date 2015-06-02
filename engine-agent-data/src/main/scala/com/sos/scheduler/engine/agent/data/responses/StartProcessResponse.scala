package com.sos.scheduler.engine.agent.data.responses

import com.sos.scheduler.engine.agent.data.AgentProcessId

/**
 * @author Joacim Zschimmer
 */
final case class StartProcessResponse(processId: AgentProcessId) extends Response
