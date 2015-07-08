package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.agent.data.responses.StartProcessResponse
import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.StringSource
import com.sos.scheduler.engine.tunnel.data.TunnelToken

/** XML response from command &lt;start_remote_task>.
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(agentProcessId: AgentProcessId, pid: Int, tunnelTokenOption: Option[TunnelToken])

object ProcessDescriptor {
  def fromXml(o: String) =
    readSchedulerResponse(StringSource(o)) { eventReader ⇒
      import eventReader._
      parseElement("process") {
        ProcessDescriptor(
          agentProcessId = AgentProcessId(attributeMap("process_id")),
          pid = attributeMap.getConverted("pid") { _.toInt } getOrElse 0,
          tunnelTokenOption = None)
      }
    }

  def fromStartProcessResponse(o: StartProcessResponse) = ProcessDescriptor(o.processId, pid = 0, o.tunnelTokenOption)
}
