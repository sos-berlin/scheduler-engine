package com.sos.scheduler.engine.plugins.jetty.tests.commons

import com.google.inject.Injector
import com.sos.scheduler.engine.plugins.jetty.WebServer
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration.TcpPortNumber
import com.sos.scheduler.engine.plugins.jetty.tests.commons.JettyPluginTests._
import java.net.URI

final class WebServiceTester(injector: Injector) {

  lazy val jettyConfiguration = JettyConfiguration(portOption = Some(TcpPortNumber.random()))

  private lazy val webServer = new WebServer(jettyConfiguration)

  def webResource =
    newAuthentifyingClient().resource(contextUri(injector))

  private def contextUri(injector: Injector) =
    new URI("http://localhost:"+ jettyPortNumber(injector) + jettyConfiguration.contextPath)

  def start() {
    webServer.start()
  }

  def close() {
    webServer.close()
  }
}
