package com.sos.scheduler.engine.plugins.jetty.configuration

import Config._
import com.sos.scheduler.engine.common.xml.XmlUtils._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.Utils._
import java.io.File
import java.net.{BindException, ServerSocket}
import org.w3c.dom.Element

final class Config(pluginElement: Element, conf: SchedulerConfiguration) {

  val portOption: Option[Int] =
    pluginElement.getAttribute("port") match {
      case "TEST" => Some(findFreePort(testPortRange))
      case _ => xmlAttributeIntOption(pluginElement, "port")
    }

  val jettyXmlFileOption: Option[File] =
    configFileIfExists("jetty.xml")

  val webXmlFileOption: Option[File] =
    configFileIfExists("web.xml")

  val accessLogFile =
    new File(conf.logDirectory, "http.log")

  //val resourceBaseOption: Option[URL] = Option(emptyToNull(conf.webDirectory())) map { f => new File(f).getAbsoluteFile.toURI.toURL }

  private def configFileIfExists(filename: String) = Option(new File(conf.mainConfigurationDirectory, filename)) filter { _.exists }
}

object Config {
  val adminstratorRoleName = "administrator"
  val contextPath = "/jobscheduler"
  val enginePrefixPath = "/engine"
  val cppPrefixPath = "/engine-cpp"
  val resourceBaseURL = getClass.getResource("/com/sos/scheduler/engine/web")
  private val testPortRange = 40000 until 50000

  //  val gzipContentTypes = List(
  //    "application/javascript",
  //    "application/xhtml+xml",
  //    "image/svg+xml",
  //    "text/css",
  //    "text/html",
  //    "text/javascript",
  //    "text/plain",
  //    "text/xml")

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
}
