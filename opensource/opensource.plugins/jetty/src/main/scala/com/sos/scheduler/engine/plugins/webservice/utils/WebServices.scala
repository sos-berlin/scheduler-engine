package com.sos.scheduler.engine.plugins.webservice.utils

import com.google.common.base.Charsets._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants._
import javax.servlet.ServletRequest
import javax.ws.rs.core.{MediaType, Variant, CacheControl}
import scala.xml.Elem

object WebServices {
  val schedulerTextPlainVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, schedulerEncoding.name())
  val textPlainVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, UTF_8.name)
  val textXmlVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, null)

  val noCache = {
    val result = new CacheControl
    result.setNoCache(true)
    result
  }

  def wrapXmlResponse(e: Seq[Elem]) = <scheduler>{e}</scheduler>

  def stripFromEnd(s: String, end: String) = s ensuring { _ endsWith end } substring (0, s.length - end.length)

  def getOrSetAttribute[A](request: ServletRequest, attributeName: String)(f: => A) =
    Option(request.getAttribute(attributeName).asInstanceOf[A]) getOrElse {
      val result = f
      request.setAttribute(attributeName, result)
      result
    }
}