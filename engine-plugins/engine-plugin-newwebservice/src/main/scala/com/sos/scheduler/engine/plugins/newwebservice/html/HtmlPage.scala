package com.sos.scheduler.engine.plugins.newwebservice.html

import java.io.StringWriter
import scala.language.implicitConversions
import spray.http.HttpCharsets.`UTF-8`
import spray.http.HttpEntity
import spray.http.MediaTypes.`text/html`
import spray.httpx.marshalling.Marshaller

/**
  * @author Joacim Zschimmer
  */
trait HtmlPage {
  def node: xml.Node
}

object HtmlPage {

  implicit val marshaller = Marshaller.of[HtmlPage](`text/html`) { (htmlPage, contentType, ctx) ⇒
    val writer = new StringWriter
    xml.XML.write(writer, htmlPage.node,
      enc = (contentType.definedCharset getOrElse `UTF-8`).toString,
      xmlDecl = false, doctype = xml.dtd.DocType("html"))
    ctx.marshalTo(HttpEntity(contentType, writer.toString))
  }
}
