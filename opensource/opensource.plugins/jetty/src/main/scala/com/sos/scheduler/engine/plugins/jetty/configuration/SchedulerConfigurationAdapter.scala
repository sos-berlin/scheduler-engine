package com.sos.scheduler.engine.plugins.jetty.configuration

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.xml.XmlUtils._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration.{TcpPortNumber, WebAppContextConfiguration}
import java.io.File
import org.w3c.dom.Element

object SchedulerConfigurationAdapter {
  def jettyConfiguration(pluginElement: Element, schedulerConfiguration: SchedulerConfiguration) = {
    val portOption = {
      val schedulerHttpPort = schedulerConfiguration.httpPort
      val portString = pluginElement.getAttribute("port")
      if (schedulerHttpPort != 0) require(portString.isEmpty, s"Either use -http-port=$schedulerHttpPort or port='$portString'")
      portString match {
        case "TEST" => Some(TcpPortNumber.lazyRandom())
        case "" if schedulerHttpPort != 0 ⇒
          Some(TcpPortNumber(schedulerHttpPort))
        case _ => xmlAttributeIntOption(pluginElement, "port") map TcpPortNumber.apply
      }
    }
    def configFileIfExists(filename: String) = Some(schedulerConfiguration.mainConfigurationDirectory / filename) filter { _.exists }
    JettyConfiguration(
      portOption = portOption,
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
