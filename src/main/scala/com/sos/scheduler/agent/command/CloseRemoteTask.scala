package com.sos.scheduler.agent.command

import com.sos.scheduler.agent.command.Parsers.parseBoolean
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader

/**
 * @author Joacim Zschimmer
 */
final case class CloseRemoteTask(processId: Long, kill: Boolean) extends Command

object CloseRemoteTask {
  def parseXml(eventReader: ScalaXMLEventReader): CloseRemoteTask = {
    import eventReader._
    parseElement() {
      CloseRemoteTask(
        processId = attributeMap.asConverted("process_id") { _.toInt },
        kill = attributeMap.getAsConverted("kill")(parseBoolean) getOrElse false
      )
    }
  }
}
