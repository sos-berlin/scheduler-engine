package com.sos.scheduler.engine.plugins.jetty.rest.bodywriters

import com.google.common.io.Closeables._
import java.net.URL
import javax.xml.transform.TransformerFactory
import javax.xml.transform.stream.{StreamResult, StreamSource}
import javax.ws.rs.core.StreamingOutput
import java.io.{OutputStream, StringReader, StringWriter}

class XsltTransformer(xsltUrl: URL) {
  private lazy val transformer = {
    val in = xsltUrl.openStream()
    try TransformerFactory.newInstance().newTransformer(new StreamSource(in))
    finally closeQuietly(in)
  }

  def transformToString(e: xml.Elem) = {
    val w = new StringWriter
    val xml = e.toString()
    transformer.transform(new StreamSource(new StringReader(xml)), new StreamResult(w))
    w.toString
  }

  def streamingOutput(e: xml.Elem) = new StreamingOutput {
    def write(out: OutputStream) {
      transform(e, out)
    }
  }

  def transform(e: xml.Elem, out: OutputStream) {
    val xml = e.toString()
    transformer.transform(new StreamSource(new StringReader(xml)), new StreamResult(out))
  }
}
