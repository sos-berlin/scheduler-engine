package com.sos.scheduler.engine.plugins.jetty.configuration.injection

import com.fasterxml.jackson.jaxrs.json.JacksonJsonProvider
import com.google.inject.Provides
import com.sos.scheduler.engine.plugins.jetty.bodywriters.BodyWriters
import com.sos.scheduler.engine.plugins.jetty.configuration.Config._
import com.sos.scheduler.engine.plugins.jetty.configuration.ObjectMapperJacksonJsonProvider
import com.sun.jersey.guice.JerseyServletModule
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer
import javax.inject.Singleton

class JerseyModule extends JerseyServletModule {
  override def configureServlets() {
    serve(s"$enginePrefixPath/*") `with` classOf[GuiceContainer]  // Route all requests through GuiceContainer
    for (c <- BodyWriters.messageBodyWriters) bind(c)
  }

  @Provides @Singleton
  def provideObjectMapperJacksonJsonProvider: JacksonJsonProvider =
    ObjectMapperJacksonJsonProvider
}