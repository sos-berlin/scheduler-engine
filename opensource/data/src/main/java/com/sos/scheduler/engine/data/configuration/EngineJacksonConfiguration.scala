package com.sos.scheduler.engine.data.configuration

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.datatype.guava.GuavaModule
import com.fasterxml.jackson.module.scala.DefaultScalaModule

object EngineJacksonConfiguration {
  def newObjectMapper() = {
    val objectMapper = new ObjectMapper
    objectMapper.registerModule(DefaultScalaModule)
    objectMapper.registerModule(new GuavaModule)
    objectMapper
  }
}
