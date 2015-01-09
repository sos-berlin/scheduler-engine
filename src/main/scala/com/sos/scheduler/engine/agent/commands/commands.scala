package com.sos.scheduler.engine.agent.commands

import com.sos.scheduler.engine.agent.common.Xmls
import com.sos.scheduler.engine.agent.common.Xmls._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.data.agent.RemoteTaskId

/**
 * @author Joacim Zschimmer
 */
sealed trait Command

object Command {
  def parseString(commandString: String): Command =
    ScalaXMLEventReader.parseString(commandString)(parseXml)

  def parseXml(eventReader: ScalaXMLEventReader): Command = {
    import eventReader._
    parseStartElementAlternative[Command] {
      case "remote_scheduler.start_remote_task" ⇒
        StartRemoteTask.parseXml(eventReader)

      case "remote_scheduler.remote_task.close" ⇒
        CloseRemoteTask.parseXml(eventReader)
    }
  }
}


/**
 * @author Joacim Zschimmer
 */
trait RemoteTaskCommand extends Command

final case class StartRemoteTask(
  controllerTcpPort: Int,
  usesApi: Boolean,
  javaOptions: String,
  javaClassPath: String)
extends RemoteTaskCommand

object StartRemoteTask {
  def parseXml(eventReader: ScalaXMLEventReader): StartRemoteTask = {
    import eventReader._
    parseElement() {
      val usesApi = attributeMap.get("kind") match {
        case Some("process") ⇒ false
        case None ⇒ true
        case x ⇒ throw new IllegalArgumentException(s"kind=$x")
      }
      StartRemoteTask(
        controllerTcpPort = attributeMap.asConverted("tcp_port") { _.toInt },
        usesApi = usesApi,
        javaOptions = attributeMap.getOrElse("java_options", ""),
        javaClassPath = attributeMap.getOrElse("java_classpath", ""))
    }
  }
}


/**
 * @author Joacim Zschimmer
 */
final case class CloseRemoteTask(remoteTaskId: RemoteTaskId, kill: Boolean)
extends RemoteTaskCommand

object CloseRemoteTask {
  def parseXml(eventReader: ScalaXMLEventReader): CloseRemoteTask = {
    import eventReader._
    parseElement() {
      CloseRemoteTask(
        remoteTaskId = attributeMap.asConverted("process_id") { o ⇒ RemoteTaskId(o.toLong) },
        kill = attributeMap.getAsConverted("kill")(xmlStringToBoolean) getOrElse false
      )
    }
  }
}

///**
// * @author Joacim Zschimmer
// */
//object Terminate
//extends Command
