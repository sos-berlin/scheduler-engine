package com.sos.scheduler.engine.plugins.jetty

import java.net.URI
import com.sun.jersey.api.client.{Client, WebResource}
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter

object JettyPluginTest {
  def newAuthentifyingResource(uri: URI) = {
    val result = Client.create()
    result.addFilter(new HTTPBasicAuthFilter("testName", "testPassword"))
    result.resource(uri)
  }

  def newAuthResource(uri: String): WebResource = newAuthentifyingResource(new URI(uri))
}
