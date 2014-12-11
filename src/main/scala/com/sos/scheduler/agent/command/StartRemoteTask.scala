package com.sos.scheduler.agent.command

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader

/**
 * @author Joacim Zschimmer
 */
final case class StartRemoteTask(
  controllerTcpPort: Int,
  usesApi: Boolean,
  javaOptions: String,
  javaClassPath: String)
  extends Command

object StartRemoteTask {

  def parseXml(eventReader: ScalaXMLEventReader): StartRemoteTask = {
    import eventReader._
    parseElement() {
      val usesApi = attributeMap.get("kind") match {
        case Some("process") ⇒ true
        case None ⇒ false
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
