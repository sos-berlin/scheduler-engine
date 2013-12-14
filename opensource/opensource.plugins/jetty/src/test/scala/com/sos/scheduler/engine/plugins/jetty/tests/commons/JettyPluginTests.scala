package com.sos.scheduler.engine.plugins.jetty.tests.commons

import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.folder.{JobPath, JobChainPath}
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.Config._
import com.sos.scheduler.engine.data.folder.{JobPath, JobChainPath}
import com.sos.scheduler.engine.plugins.jetty.WebServer
import com.sos.scheduler.engine.plugins.jetty.configuration.Config._
import com.sos.scheduler.engine.plugins.jetty.configuration.ObjectMapperJacksonJsonProvider
import com.sun.jersey.api.client.config.DefaultClientConfig
import com.sun.jersey.api.client.filter.{ClientFilter, HTTPBasicAuthFilter}
import com.sun.jersey.api.client.filter.{ClientFilter, HTTPBasicAuthFilter}
import com.sun.jersey.api.client.{Client, WebResource}
import java.net.URI
import org.joda.time.Duration

object JettyPluginTests {

  private val defaultTimeout = 60.s
  val aJobChainPath = JobChainPath("/a")
  val orderJobPath = JobPath("/order")

  def javaResource(injector: Injector) =
    newAuthentifyingClient().resource(javaContextUri(injector))

  private def javaContextUri(injector: Injector) =
    new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath + enginePrefixPath)

  def contextUri(injector: Injector) =
    new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath)

  def jettyPortNumber(injector: Injector) =
    WebServer.tcpPortNumber
    //Macht abhängig vom Scheduler: injector.getInstance(classOf[PluginSubsystem]).pluginByClass(classOf[JettyPlugin]).tcpPortNumber

  def newAuthResource(uri: URI): WebResource = {
    val client = newAuthentifyingClient(defaultTimeout)
    client.resource(uri)
  }

  def newAuthentifyingClient(timeout: Duration = defaultTimeout, filters: Iterable[ClientFilter] = Iterable()) = {
    val config = new DefaultClientConfig sideEffect { _.getSingletons.add(ObjectMapperJacksonJsonProvider) }
    Client.create(config) sideEffect { client =>
      client.setReadTimeout(timeout.getMillis.toInt)
      client.addFilter(new HTTPBasicAuthFilter("testName", "testPassword"))
      for (f <- filters) client.addFilter(f)
    }
  }
}
