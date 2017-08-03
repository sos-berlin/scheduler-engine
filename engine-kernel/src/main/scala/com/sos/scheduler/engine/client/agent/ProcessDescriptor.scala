package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.data.AgentTaskId
import com.sos.scheduler.engine.agent.data.commandresponses.StartTaskSucceeded
import com.sos.scheduler.engine.tunnel.data.TunnelToken

/**
 * XML response from command &lt;start_remote_task>.
 *
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(agentTaskId: AgentTaskId, pid: Int, tunnelToken: TunnelToken)

object ProcessDescriptor {
  def fromStartProcessResponse(o: StartTaskSucceeded) = ProcessDescriptor(o.agentTaskId, pid = 0, o.tunnelToken)
}
