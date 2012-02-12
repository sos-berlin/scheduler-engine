package com.sos.scheduler.engine.plugins.jetty.util

import java.net.URI
import com.google.inject.Injector
import com.sun.jersey.api.client.{Client, WebResource}
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.Config._
import org.joda.time.Duration
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin

object JettyPluginTests {
  private val defaultTimeout = new Duration(60 * 1000)

  def cppContextUri(injector: Injector) = new URI("http://localhost:" + jettyPortNumber(injector) + contextPath + cppPrefixPath)

  def javaContextUri(injector: Injector) = new URI("http://localhost:" + jettyPortNumber(injector) + contextPath + enginePrefixPath)

  def contextUri(injector: Injector) = new URI("http://localhost:" + jettyPortNumber(injector) + contextPath)

  private def jettyPortNumber(injector: Injector) = injector.getInstance(classOf[PluginSubsystem]).pluginByClass(classOf[JettyPlugin]).port

  def newAuthResource(uri: String): WebResource = newAuthentifyingResource(new URI(uri))

  private def newAuthentifyingResource(uri: URI, timeout: Duration = defaultTimeout) = {
    val client = newAuthentifyingClient(timeout)
    client.resource(uri)
  }

  def newAuthentifyingClient(timeout: Duration = defaultTimeout) = {
    val result = Client.create()
    result.setReadTimeout(timeout.getMillis.toInt)
    result.addFilter(new HTTPBasicAuthFilter("testName", "testPassword"))
    result
  }
}
