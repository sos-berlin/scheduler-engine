package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId

/**
 * @author Joacim Zschimmer
 */
final case class StartProcessResponse(processId: AgentProcessId) extends Response
