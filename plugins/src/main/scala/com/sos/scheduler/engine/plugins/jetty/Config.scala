package com.sos.scheduler.engine.plugins.jetty

import java.io.File
import com.sos.scheduler.engine.kernel.util.XmlUtils._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import org.w3c.dom.Element

class Config(pluginElement: Element, configuration: SchedulerConfiguration) {
  import Config._

  val portOption = xmlAttributeIntOption(pluginElement, "port")
  val tryUntilPortOption = xmlAttributeIntOption(pluginElement, "tryUntilPort")
  val jettyXmlFileOption = Option(new File(configuration.localConfigurationDirectory, "jetty.xml")) filter { _.exists }
  val accessLogFile = new File(configuration.logDirectory, "http.log")
}

object Config {
  val adminstratorRoleName = "administrator"
  val contextPath = "/JobScheduler"
  val enginePrefixPath = "/engine"
  val cppPrefixPath = "/engine-cpp"

  //  val gzipContentTypes = List(
  //    "application/javascript",
  //    "application/xhtml+xml",
  //    "image/svg+xml",
  //    "text/css",
  //    "text/html",
  //    "text/javascript",
  //    "text/plain",
  //    "text/xml")

  private def xmlAttributeIntOption(e: Element, name: String) = if (e.hasAttribute(name)) Some(intXmlAttribute(e, name)) else None
}