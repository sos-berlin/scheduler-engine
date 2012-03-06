package com.sos.scheduler.engine.plugins.jetty.rest.views

import com.sos.scheduler.engine.plugins.jetty.rest.marshal.Marshaller
import org.codehaus.jackson.JsonGenerator

object RootViewMarshaller extends Marshaller[RootView] {
  val objectClass = classOf[RootView]

  def marshal(j: JsonGenerator, o: RootView) {
    j.writeStartObject()
    j.writeObjectFieldStart("folders")
    for (a <- o.folderUris) j.writeStringField(a._1.name, a._2.toString)
    j.writeEndObject()
    //j.writeStringField("configurationUri", o.configurationUri.toString)
    j.writeStringField("logUri", o.logUri.toString)
    j.writeEndObject()
  }
}
