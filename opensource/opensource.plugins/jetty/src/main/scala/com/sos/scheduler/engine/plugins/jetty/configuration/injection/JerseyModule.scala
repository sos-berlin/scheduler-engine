package com.sos.scheduler.engine.plugins.jetty.configuration.injection

import com.google.inject.{Provides, AbstractModule}
import com.sos.scheduler.engine.data.configuration.EngineJacksonConfiguration.newObjectMapper
import com.sos.scheduler.engine.plugins.jetty.bodywriters.BodyWriters
import com.sos.scheduler.engine.plugins.jetty.configuration.ObjectMapperJacksonJsonProvider
import javax.inject.Singleton
import com.fasterxml.jackson.jaxrs.json.JacksonJsonProvider

final class JerseyModule extends AbstractModule {
  override def configure() {
    for (c <- BodyWriters.messageBodyWriters) bind(c)
  }

  @Provides @Singleton
  def provideObjectMapperJacksonJsonProvider: JacksonJsonProvider =
    ObjectMapperJacksonJsonProvider
}
