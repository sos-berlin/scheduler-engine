package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.agent.data.responses.StartProcessResponse
import com.sos.scheduler.engine.tunnel.data.TunnelToken

/** XML response from command &lt;start_remote_task>.
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(agentProcessId: AgentProcessId, pid: Int, tunnelToken: TunnelToken)

object ProcessDescriptor {
  def fromStartProcessResponse(o: StartProcessResponse) = ProcessDescriptor(o.processId, pid = 0, o.tunnelToken)
}
