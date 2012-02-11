package com.sos.scheduler.engine.plugins.jetty

import java.net.URI
import com.google.inject.Injector
import com.sun.jersey.api.client.{Client, WebResource}
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.Config._
import org.joda.time.Duration

object JettyPluginTests {
  val defaultTimeout = new Duration(60*1000)

  def cppContextUri(injector: Injector) = new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath + cppPrefixPath)

  def javaContextUri(injector: Injector) = new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath + enginePrefixPath)

  def jettyPortNumber(injector: Injector) = {
    val pluginSubsystem = injector.getInstance(classOf[PluginSubsystem])
    val jettyPlugin = pluginSubsystem.pluginByClass(classOf[JettyPlugin])
    jettyPlugin.port
  }

  def newAuthentifyingClient(timeout: Duration = defaultTimeout) = {
    val result = Client.create()
    result.setReadTimeout(timeout.getMillis.toInt)
    result.addFilter(new HTTPBasicAuthFilter("testName", "testPassword"))
    result
  }

  def newAuthentifyingResource(uri: URI, timeout: Duration = defaultTimeout) = {
    val client = newAuthentifyingClient(timeout)
    client.resource(uri)
  }

  def newAuthResource(uri: String): WebResource = newAuthentifyingResource(new URI(uri))
}
