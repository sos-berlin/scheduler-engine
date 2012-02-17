package com.sos.scheduler.engine.plugins.jetty.rest.transform

import scala.xml.Elem
import java.lang.reflect.Type
import java.lang.annotation.Annotation
import java.io.{OutputStreamWriter, OutputStream}
import javax.inject.Singleton
import javax.ws.rs.Produces
import javax.ws.rs.core.{MultivaluedMap, MediaType}
import javax.ws.rs.core.MediaType._
import javax.ws.rs.ext.MessageBodyWriter
import javax.ws.rs.ext.Provider
import com.google.common.base.Charsets.UTF_8

@Provider
@Produces(Array(TEXT_XML))
@Singleton
class XmlElemWriter extends MessageBodyWriter[Elem] {
  def isWriteable(c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType) =
    c == classOf[Elem] && mediaType == TEXT_XML_TYPE

  def getSize(o: Elem, c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType) = -1

  def writeTo(o: Elem, c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType,
      httpHeaders: MultivaluedMap[String, AnyRef], out: OutputStream) {
    val encoding = UTF_8
    // TODO encoding im Header setzen?
    val w = new OutputStreamWriter(out, encoding)
    w.write("<?xml version='1.0'?>")  // encoding='"+encoding+"'?>")
    w.write(o.toString())
    w.close()
  }
}