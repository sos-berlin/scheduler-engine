package com.sos.scheduler.engine.kernel.agentclient

import com.sos.scheduler.engine.common.scalautil.xml.StringSource
import com.sos.scheduler.engine.kernel.agentclient.RemoteSchedulers._

/** XML response from command &lt;start_remote_task>.
 * @author Joacim Zschimmer
 */
final case class ProcessDescriptor(processId: String, pid: Int)

object ProcessDescriptor {
  def fromXml(o: String) =
    readSchedulerResponse(StringSource(o)) { eventReader ⇒
      import eventReader._
      var processId: String = null
      var pid: Option[Int] = None

      parseElement("process") {
        forEachAttribute {
          case ("process_id", value) ⇒ processId = value
          case ("pid", value) ⇒ pid = Some(value.toInt)
          case _ ⇒
        }
      }
      require(processId != null, "Attribute 'process_id' is missing")
      ProcessDescriptor(processId = processId, pid = pid getOrElse 0)
    }
}
