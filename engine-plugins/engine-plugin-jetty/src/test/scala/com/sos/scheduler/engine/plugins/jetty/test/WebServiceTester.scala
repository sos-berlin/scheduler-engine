package com.sos.scheduler.engine.plugins.jetty.test

import com.google.inject.Injector
import com.sos.scheduler.engine.plugins.jetty.WebServer
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration.LazyRandomTcpPortNumber
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._
import java.net.URI

final class WebServiceTester(injector: Injector) {

  lazy val jettyConfiguration = JettyConfiguration(portOption = Some(new LazyRandomTcpPortNumber))
  lazy val portNumber = jettyConfiguration.portOption.get.value

  private lazy val webServer = new WebServer(jettyConfiguration)

  def webResource = newAuthentifyingClient().resource(new URI(s"http://127.0.0.1:$portNumber"))

  def start(): Unit = {
    webServer.start()
  }

  def close(): Unit = {
    webServer.close()
  }
}
