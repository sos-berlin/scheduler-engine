package com.sos.scheduler.engine.plugins.jetty.rest.views

import com.sos.scheduler.engine.plugins.jetty.rest.marshal.Marshaller
import org.codehaus.jackson.JsonGenerator

object FolderViewMarshaller extends Marshaller[FolderView] {
  val objectClass = classOf[FolderView]

  def marshal(j: JsonGenerator, o: FolderView) {
    j.writeStartObject()
    j.writeStringField("folder", o.folderPath.getString)
    j.writeStringField("type", o.typeName)
    j.writeArrayFieldStart("entries")
    for (e <- o.entries)  marshalEntry(j, e)
    j.writeEndArray()
    j.writeEndObject()
  }

  private def marshalEntry(j: JsonGenerator, o: FolderView#Entry) {
    j.writeStartObject()
    j.writeStringField("name", o.name)
    j.writeStringField("uri", o.uri.toString)
    j.writeEndObject()
  }
}
