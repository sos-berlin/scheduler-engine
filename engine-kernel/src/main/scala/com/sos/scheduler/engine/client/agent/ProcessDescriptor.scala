package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.StringSource
import com.sos.scheduler.engine.data.agent.RemoteTaskId

/** XML response from command &lt;start_remote_task>.
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(remoteTaskId: RemoteTaskId, pid: Int)

object ProcessDescriptor {
  def fromXml(o: String) =
    readSchedulerResponse(StringSource(o)) { eventReader ⇒
      import eventReader._
      parseElement("process") {
        ProcessDescriptor(
          remoteTaskId = RemoteTaskId(attributeMap("process_id").toLong),
          pid = attributeMap.getConverted("pid") { _.toInt } getOrElse 0)
      }
    }
}
