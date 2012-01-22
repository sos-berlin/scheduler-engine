package com.sos.scheduler.engine.plugins.jetty

import java.net.URI
import com.sun.jersey.api.client.{Client, WebResource}
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter
import org.joda.time.Duration

object JettyPluginTest {
  val defaultTimeout = new Duration(60*1000)

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
