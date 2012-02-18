package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.kernel.util.XmlUtils._
import java.io.File
import org.w3c.dom.Element

class Config(pluginElement: Element, conf: SchedulerConfiguration) {
  import Config._

  val portOption: Option[Int] = xmlAttributeIntOption(pluginElement, "port")
  val tryUntilPortOption: Option[Int] = xmlAttributeIntOption(pluginElement, "tryUntilPort")
  val jettyXmlFileOption: Option[File] = configFileIfExists("jetty.xml")
  val webXmlFileOption: Option[File] = configFileIfExists("web.xml")
  val accessLogFile = new File(conf.logDirectory, "http.log")
  //val resourceBaseOption: Option[URL] = Option(emptyToNull(conf.webDirectory())) map { f => new File(f).getAbsoluteFile.toURI.toURL }

  private def configFileIfExists(filename: String) = Option(new File(conf.localConfigurationDirectory, filename)) filter { _.exists }
}

object Config {
  val adminstratorRoleName = "administrator"
  val contextPath = "/jobscheduler"
  val enginePrefixPath = "/engine"
  val cppPrefixPath = "/engine-cpp"
  val resourceBaseURL = getClass.getResource("/com/sos/scheduler/engine/web")


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
