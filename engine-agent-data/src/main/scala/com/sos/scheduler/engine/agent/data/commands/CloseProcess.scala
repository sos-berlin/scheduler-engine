package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.agent.data.responses.EmptyResponse

/**
 * @author Joacim Zschimmer
 */
final case class CloseProcess(processId: AgentProcessId, kill: Boolean)
extends ProcessCommand {
  type Response = EmptyResponse.type
}

object CloseProcess {
  val XmlElementName = "remote_scheduler.remote_task.close"
}
