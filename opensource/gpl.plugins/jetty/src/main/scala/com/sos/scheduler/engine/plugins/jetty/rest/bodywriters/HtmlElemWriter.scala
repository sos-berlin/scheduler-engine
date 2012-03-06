package com.sos.scheduler.engine.plugins.jetty.rest.bodywriters

import com.sos.scheduler.engine.plugins.jetty.rest.annotations.HtmlXsltResource
import java.lang.annotation.Annotation
import java.lang.reflect.Type
import java.io.OutputStream
import javax.inject.Singleton
import javax.ws.rs.Produces
import javax.ws.rs.core.{MultivaluedMap, MediaType}
import javax.ws.rs.core.MediaType._
import javax.ws.rs.ext.MessageBodyWriter
import javax.ws.rs.ext.Provider
import scala.collection.mutable
import scala.xml.Elem

@Provider
@Produces(Array(TEXT_HTML))
@Singleton
class HtmlElemWriter extends MessageBodyWriter[Elem] {
  import HtmlElemWriter._

  def isWriteable(c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType) =
    c == classOf[Elem] &&
    mediaType == TEXT_HTML_TYPE &&
    (annotations exists { _.annotationType == classOf[HtmlXsltResource] })

  def getSize(o: Elem, c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType) = -1

  def writeTo(o: Elem, c: Class[_], genericType: Type, annotations: Array[Annotation], mediaType: MediaType,
              httpHeaders: MultivaluedMap[String, AnyRef], out: OutputStream) {
    val resourcePath = (annotations collect { case a: HtmlXsltResource => a.path() }).head
    val t = transformerCache.getOrElseUpdate(resourcePath, new XsltTransformer(getClass.getClassLoader.getResource(resourcePath)))
    t.transform(o, out)
  }
}

object HtmlElemWriter {
  private val transformerCache = new mutable.HashMap[String,XsltTransformer] with mutable.SynchronizedMap[String,XsltTransformer]
}
