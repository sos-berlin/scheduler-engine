package com.sos.scheduler.engine.plugins.jetty.configuration

import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration._
import java.io.File
import java.net.URL
import org.eclipse.jetty.security.LoginService
import org.eclipse.jetty.server.Handler
import org.eclipse.jetty.servlet.ServletContextHandler
import scala.collection.immutable

final case class JettyConfiguration(
  portOption: Option[TcpPortNumber] = None,
  contextPath: String = Config.contextPath,
  webAppContextConfigurationOption: Option[WebAppContextConfiguration] = None,
  jettyXMLURLOption: Option[URL] = None,
  accessLogFileOption: Option[File] = None,
  loginServiceOption: Option[LoginService] = None,
  handlers: immutable.Seq[Handler] = Nil,
  servletContextHandlerModifiers: immutable.Seq[ServletContextHandler ⇒ Unit] = Nil,
  gzip: Boolean = true
)

object JettyConfiguration {

  final case class WebAppContextConfiguration(
    resourceBaseURL: URL,
    webXMLFileOption: Option[File] = None)

  trait TcpPortNumber {
    def value: Int
  }

  object TcpPortNumber {
    def apply(o: Int) = new TcpPortNumber {
      val value = o
    }
  }

  final case class FixedTcpPortNumber(value: Int) extends TcpPortNumber

  final class LazyRandomTcpPortNumber extends TcpPortNumber {
    lazy val value = findRandomFreeTcpPort()
  }
}
