package com.sos.scheduler.engine.plugins.newwebservice

import spray.http.ContentType
import spray.http.MediaTypes._
import spray.routing.directives.ContentTypeResolver

private[newwebservice] object OurContentTypeResolver extends ContentTypeResolver {

  private val ExtenstionToMediaType = Map(
    "xsl" → `text/xml`
  )

  def apply(filename: String): ContentType = {
    val myContentTypeOption = filename lastIndexOf '.' match {
      case -1 ⇒ None
      case i ⇒ ExtenstionToMediaType.get(filename.substring(i + 1)) map ContentType.apply
    }
    myContentTypeOption getOrElse ContentTypeResolver.Default(filename)
  }
}
