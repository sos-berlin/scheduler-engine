package com.sos.scheduler.engine.plugins.jetty

import java.net.URI
import com.google.inject.Injector
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.Config._
import com.sun.jersey.api.client.{Client, WebResource}
import com.sun.jersey.api.client.filter.{ClientFilter, HTTPBasicAuthFilter}
import org.joda.time.Duration

object JettyPluginTests {
  private val defaultTimeout = new Duration(60 * 1000)

  def javaResource(injector: Injector) = newAuthentifyingClient().resource(javaContextUri(injector))

  private def javaContextUri(injector: Injector) = new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath + enginePrefixPath)

  def contextUri(injector: Injector) = new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath)

  def jettyPortNumber(injector: Injector) = injector.getInstance(classOf[PluginSubsystem]).pluginByClass(classOf[JettyPlugin]).tcpPortNumber

  def newAuthResource(uri: URI): WebResource = {
    val client = newAuthentifyingClient(defaultTimeout)
    client.resource(uri)
  }

  def newAuthentifyingClient(timeout: Duration = defaultTimeout, filters: Iterable[ClientFilter] = Iterable()) = {
    val result = Client.create()
    result.setReadTimeout(timeout.getMillis.toInt)
    result.addFilter(new HTTPBasicAuthFilter("testName", "testPassword"))
    for (f <- filters) result.addFilter(f)
    result
  }
}
