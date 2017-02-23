package com.sos.scheduler.engine.client.agent

import com.sos.jobscheduler.agent.data.AgentTaskId
import com.sos.jobscheduler.agent.data.commandresponses.StartTaskResponse
import com.sos.jobscheduler.tunnel.data.TunnelToken

/**
 * XML response from command &lt;start_remote_task>.
 *
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(agentTaskId: AgentTaskId, pid: Int, tunnelToken: TunnelToken)

object ProcessDescriptor {
  def fromStartProcessResponse(o: StartTaskResponse) = ProcessDescriptor(o.agentTaskId, pid = 0, o.tunnelToken)
}
