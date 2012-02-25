package com.sos.scheduler.engine.plugins.jetty.rest.views

import com.sos.scheduler.engine.plugins.jetty.rest.marshal.Marshaller
import org.codehaus.jackson.JsonGenerator

object JobViewMarshaller extends Marshaller[JobView] {
  val objectClass = classOf[JobView]

  def marshal(j: JsonGenerator, o: JobView) {
    j.writeStartObject()
    j.writeStringField("descriptionUri", o.descriptionUri.toString)
    j.writeStringField("configurationUri", o.configurationUri.toString)
    j.writeStringField("logSnapshotUri", o.logSnapshotUri.toString)
    j.writeEndObject()
  }
}
