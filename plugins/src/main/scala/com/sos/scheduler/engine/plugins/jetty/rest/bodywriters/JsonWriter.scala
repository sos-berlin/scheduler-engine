package com.sos.scheduler.engine.plugins.jetty.rest.bodywriters

import java.lang.annotation.Annotation
import java.lang.reflect.Type
import java.io.OutputStream
import javax.ws.rs.Produces
import javax.ws.rs.core.{MultivaluedMap, MediaType}
import javax.ws.rs.core.MediaType._
import javax.ws.rs.ext.MessageBodyWriter
import javax.ws.rs.ext.Provider
import com.sos.scheduler.engine.plugins.jetty.rest.marshal.MarshallerRegister
import javax.inject.{Inject, Singleton}

@Provider
@Produces(Array(APPLICATION_JSON))
@Singleton
class JsonWriter @Inject()(marshallerRegister: MarshallerRegister) extends MessageBodyWriter[AnyRef] {
  def isWriteable(c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType) =
    mediaType == APPLICATION_JSON_TYPE && marshallerRegister.marshallerOption(c).isDefined

  def getSize(o: AnyRef, c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType) = -1

  def writeTo(o: AnyRef, c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType,
              httpHeaders: MultivaluedMap[String, AnyRef], out: OutputStream) {
    marshallerRegister.marshalToJson(o, out)
  }
}
