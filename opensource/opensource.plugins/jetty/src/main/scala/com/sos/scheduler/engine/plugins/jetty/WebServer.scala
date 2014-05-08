package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.plugins.jetty.JettyServerBuilder.newJettyServer
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration
import org.eclipse.jetty.server.{Connector, Server}

class WebServer(jettyConfiguration: JettyConfiguration) {
  private var started = false
  private val jettyServer: Server = newJettyServer(jettyConfiguration)

  private val connectors: Seq[Connector] = for (cs <- jettyServer.getConnectors) yield cs
  val portNumbers: Seq[Int] = connectors map { _.getPort } // Der Port des ersten Connector oder None

  def start() {
    jettyServer.start()
    started = true
  }

  def close() {
    jettyServer.stop()
    if (started) {
      jettyServer.join()
      started = false
    }
  }
}
