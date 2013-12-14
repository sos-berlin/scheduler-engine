package com.sos.scheduler.engine.plugins.jetty.configuration

import com.sos.scheduler.engine.common.xml.XmlUtils._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration.{TcpPortNumber, WebAppContextConfiguration}
import java.io.File
import org.w3c.dom.Element

object SchedulerConfigurationAdapter {
  def jettyConfiguration(pluginElement: Element, schedulerConfiguration: SchedulerConfiguration) = {
    def configFileIfExists(filename: String) =
      Some(new File(schedulerConfiguration.mainConfigurationDirectory, filename)) filter { _.exists }

    JettyConfiguration(
      contextPath = Config.contextPath,
      portOption = pluginElement.getAttribute("port") match {
        case "TEST" => Some(TcpPortNumber.random())
        case _ => xmlAttributeIntOption(pluginElement, "port") map TcpPortNumber.apply
      },
      jettyXMLURLOption = configFileIfExists("jetty.xml") map { _.toURI.toURL },
      webAppContextConfigurationOption = Some(WebAppContextConfiguration(
        resourceBaseURL = Config.resourceBaseURL,
        webXMLFileOption = configFileIfExists("web.xml"))),
      loginServiceOption = childElementOption(pluginElement, "loginService") map PluginLoginService.apply,
      accessLogFileOption = Some(new File(schedulerConfiguration.logDirectory, "http.log"))
    )
  }

  private def xmlAttributeIntOption(e: Element, name: String) =
    if (e.hasAttribute(name)) Some(intXmlAttribute(e, name)) else None
}
