package com.sos.scheduler.engine.agent.xmlcommand

import com.sos.scheduler.engine.agent.commands.CloseRemoteTask
import com.sos.scheduler.engine.agentcommon.Xmls._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.data.agent.RemoteTaskId

/**
 * @author Joacim Zschimmer
 */
object CloseRemoteTaskXml {
  def parseCommandXml(eventReader: ScalaXMLEventReader): CloseRemoteTask = {
    import eventReader._
    parseElement() {
      CloseRemoteTask(
        remoteTaskId = attributeMap.asConverted("process_id") { o ⇒ RemoteTaskId(o.toLong) },
        kill = attributeMap.getAsConverted("kill")(xmlStringToBoolean) getOrElse false
      )
    }
  }

  def responseXmlElem: xml.Elem = <ok/>
}
