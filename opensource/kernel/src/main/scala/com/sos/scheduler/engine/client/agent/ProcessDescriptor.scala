package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.common.scalautil.xmls.StringSource

/** XML response from command &lt;start_remote_task>.
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(processId: String, pid: Int)

object ProcessDescriptor {
  def fromXml(o: String) =
    readSchedulerResponse(StringSource(o)) { eventReader ⇒
      import eventReader._
      parseElement("process") {
        ProcessDescriptor(
          processId = attributeMap("process_id"),
          pid = attributeMap.asConverted("pid") { _.toInt } getOrElse 0)
      }
    }
}
