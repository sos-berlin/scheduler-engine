package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId

/**
 * @author Joacim Zschimmer
 */
/**
 * @author Joacim Zschimmer
 */
final case class CloseProcess(processId: AgentProcessId, kill: Boolean)
  extends ProcessCommand {
  type Response = CloseProcessResponse.type
}

object CloseProcess {
  val XmlElementName = "remote_scheduler.remote_task.close"
}
