package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.StringSource
import com.sos.scheduler.engine.data.agent.AgentProcessId

/** XML response from command &lt;start_remote_task>.
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(agentProcessId: AgentProcessId, pid: Int)

object ProcessDescriptor {
  def fromXml(o: String) =
    readSchedulerResponse(StringSource(o)) { eventReader ⇒
      import eventReader._
      parseElement("process") {
        ProcessDescriptor(
          agentProcessId = AgentProcessId(attributeMap("process_id").toLong),
          pid = attributeMap.getConverted("pid") { _.toInt } getOrElse 0)
      }
    }
}
