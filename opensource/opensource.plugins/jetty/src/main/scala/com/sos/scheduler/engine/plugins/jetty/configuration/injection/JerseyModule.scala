package com.sos.scheduler.engine.plugins.jetty.configuration.injection

import JerseyModule._
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.scala.DefaultScalaModule
import com.google.inject.{Provides, Provider, AbstractModule}
import com.sos.scheduler.engine.plugins.jetty.bodywriters.BodyWriters
import com.sos.scheduler.engine.plugins.jetty.configuration.ObjectMapperJacksonJsonProvider
import javax.inject.Singleton

final class JerseyModule extends AbstractModule {
  override def configure() {
    for (c <- BodyWriters.messageBodyWriters) bind(c)
  }

  @Provides @Singleton
  def provideObjectMapperJacksonJsonProvider: ObjectMapperJacksonJsonProvider =
    new ObjectMapperJacksonJsonProvider(newObjectMapper())
}

object JerseyModule {
  private def newObjectMapper() = {
    val result = new ObjectMapper
    result.registerModule(DefaultScalaModule)
    result
  }
}
