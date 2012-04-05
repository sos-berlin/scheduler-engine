package com.sos.scheduler.engine.plugins.jetty.rest.views

import org.codehaus.jackson.JsonGenerator
import org.codehaus.jackson.map.{SerializerProvider, JsonSerializer}

class RootViewSerializer extends JsonSerializer[RootView] {
  //override def handledType = classOf[RootView]

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
