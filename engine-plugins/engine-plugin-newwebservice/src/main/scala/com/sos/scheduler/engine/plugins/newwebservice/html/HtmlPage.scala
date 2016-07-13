package com.sos.scheduler.engine.plugins.newwebservice.html

import scala.language.implicitConversions
import scalatags.Text.TypedTag
import spray.http.HttpEntity
import spray.http.MediaTypes.`text/html`
import spray.httpx.marshalling.Marshaller

/**
  * @author Joacim Zschimmer
  */
trait HtmlPage {
  def scalatag: TypedTag[String]
}

object HtmlPage {

  implicit val marshaller = Marshaller.of[HtmlPage](`text/html`) { (htmlPage, contentType, ctx) ⇒
    val sb = new StringBuilder(10000)
    sb.append("<!DOCTYPE html>")
    htmlPage.scalatag.writeTo(sb)
    ctx.marshalTo(HttpEntity(contentType, sb.toString))
  }
}
