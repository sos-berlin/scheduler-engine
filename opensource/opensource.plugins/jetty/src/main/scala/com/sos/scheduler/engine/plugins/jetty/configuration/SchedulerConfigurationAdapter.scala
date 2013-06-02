package com.sos.scheduler.engine.plugins.jetty.configuration

import com.sos.scheduler.engine.common.xml.XmlUtils._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.utils.Utils.randomInts
import java.io.File
import java.net.{BindException, ServerSocket}
import org.w3c.dom.Element

object SchedulerConfigurationAdapter {
  private val testPortRange = 40000 until 50000

  def serverConfiguration(pluginElement: Element, schedulerConfiguration: SchedulerConfiguration) = {
    def configFileIfExists(filename: String) =
      Some(new File(schedulerConfiguration.mainConfigurationDirectory, filename)) filter { _.exists }

    ServerConfiguration(
      contextPathOption = Some(Config.contextPath),
      portOption = pluginElement.getAttribute("port") match {
        case "TEST" => Some(findFreePort(testPortRange))
        case _ => xmlAttributeIntOption(pluginElement, "port")
      },
      jettyXMLURLOption = configFileIfExists("jetty.xml") map { _.toURI.toURL },
      webXMLFileOption = configFileIfExists("web.xml"),
      loginServiceOption = childElementOption(pluginElement, "loginService") map PluginLoginService.apply,
      accessLogFileOption = Some(new File(schedulerConfiguration.logDirectory, "http.log")),
      resourceBaseURLOption = Some(Config.resourceBaseURL)
    )
  }

  private def xmlAttributeIntOption(e: Element, name: String) =
    if (e.hasAttribute(name)) Some(intXmlAttribute(e, name)) else None

  private def findFreePort(range: Range): Int =
    findFreePort(randomInts(range)) getOrElse range.head

  private def findFreePort(ports: TraversableOnce[Int]): Option[Int] =
    ports.toIterator find portIsFree

  private def portIsFree(port: Int) =
    try {
      val backlog = 1
      new ServerSocket(port, backlog).close()
      true
    } catch {
      case _: BindException => false
    }

  private def childElementOption(e: Element, name: String) =
    Option(childElementOrNull(e, name))
}
