package com.sos.scheduler.engine.plugins.jetty.configuration.injection

import com.sos.scheduler.engine.plugins.jetty.configuration.Config._
import com.sun.jersey.guice.JerseyServletModule
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer

class JerseyModule extends JerseyServletModule {
  override def configureServlets(): Unit = {
    serve(s"$enginePrefixPath/*") `with` classOf[GuiceContainer]  // Route all requests through GuiceContainer
  }
}
