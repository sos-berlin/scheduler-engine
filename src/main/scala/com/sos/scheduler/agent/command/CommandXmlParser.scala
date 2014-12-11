package com.sos.scheduler.agent.command

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader

/**
 * @author Joacim Zschimmer
 */
object CommandXmlParser {

  def parseCommand(commandString: String): Command =
    ScalaXMLEventReader.parseString(commandString) { eventReader ⇒
      import eventReader._
      parseStartElementAlternative[Command] {
        case "remote_scheduler.start_remote_task" ⇒
          StartRemoteTask.parseXml(eventReader)
        case "remote_scheduler.remote_task.close" ⇒
          CloseRemoteTask.parseXml(eventReader)
      }
    }
}
