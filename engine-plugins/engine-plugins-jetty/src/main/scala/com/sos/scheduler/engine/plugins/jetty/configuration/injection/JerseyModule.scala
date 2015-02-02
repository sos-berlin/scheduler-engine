package com.sos.scheduler.engine.plugins.jetty.configuration.injection

import com.fasterxml.jackson.jaxrs.json.JacksonJsonProvider
import com.google.inject.Provides
import com.sos.scheduler.engine.plugins.jetty.configuration.Config._
import com.sos.scheduler.engine.plugins.jetty.configuration.ObjectMapperJacksonJsonProvider
import com.sun.jersey.guice.JerseyServletModule
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer
import javax.inject.Singleton

class JerseyModule extends JerseyServletModule {
  override def configureServlets(): Unit = {
    serve(s"$enginePrefixPath/*") `with` classOf[GuiceContainer]  // Route all requests through GuiceContainer
  }

  @Provides @Singleton
  def provideObjectMapperJacksonJsonProvider: JacksonJsonProvider =
    ObjectMapperJacksonJsonProvider
}
