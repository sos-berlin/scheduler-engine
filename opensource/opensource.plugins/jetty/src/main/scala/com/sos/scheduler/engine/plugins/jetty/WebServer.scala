package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import com.sos.scheduler.engine.plugins.jetty.JettyServerBuilder.newJettyServer
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration
import org.eclipse.jetty.server.{Connector, Server}
import scala.collection.immutable

final class WebServer(jettyConfiguration: JettyConfiguration) extends HasCloser {

  private val jettyServer: Server = newJettyServer(jettyConfiguration)
  val portNumbers: immutable.Seq[Int] = (connectors map { _.getPort } filter { _ != 0 }).toImmutableSeq

  private def connectors: Seq[Connector] = Option(jettyServer.getConnectors) map { _.toSeq } getOrElse Nil

  def start() {
    jettyServer.start()
    onClose {
      jettyServer.stop()
      jettyServer.join()
    }
  }
}
