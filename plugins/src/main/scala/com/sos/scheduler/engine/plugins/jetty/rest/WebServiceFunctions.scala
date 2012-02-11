package com.sos.scheduler.engine.plugins.jetty.rest

import javax.servlet.ServletRequest
import javax.ws.rs.core.CacheControl
import scala.xml.Elem

object WebServiceFunctions {
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