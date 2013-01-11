package com.sos.scheduler.engine.plugins.jetty.rest

import com.fasterxml.jackson.core.JsonGenerator
import com.fasterxml.jackson.databind.{SerializerProvider, JsonSerializer}

class RootViewSerializer extends JsonSerializer[RootView] {
  def serialize(o: RootView, j: JsonGenerator, provider: SerializerProvider) {
    j.writeStartObject()
    //j.writeStringField("configurationUri", o.configurationUri.toString)
    j.writeStringField("logUri", o.logUri.toString)
    j.writeObjectFieldStart("folders")
    for (a <- o.folders) j.writeStringField(a._1, a._2.toString)
    j.writeEndObject()
    j.writeEndObject()
  }
}
