package com.sos.scheduler.engine.plugins.jetty

import javax.ws.rs.core.CacheControl
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants
import javax.ws.rs.core.Response.ResponseBuilder
import scala.xml.Elem
import javax.servlet.ServletRequest

object WebServiceFunctions {
  val noCache = {
    val result = new CacheControl
    result.setNoCache(true)
    result
  }

  object HeaderConstants {
    val server = SchedulerConstants.productWithVersion
  }

  //def finishResponse(r: ResponseBuilder) = r.header("Server", HeaderConstants.server).build()

  def wrapXmlResponse(e: Seq[Elem]) = <scheduler>{e}</scheduler>

  def stripFromEnd(s: String, end: String) = s ensuring {_ endsWith end} substring (0, s.length - end.length)

  def getOrSetAttribute[A](request: ServletRequest, attributeName: String)(f: => A) =
    Option(request.getAttribute(attributeName).asInstanceOf[A]) getOrElse {
      val result = f
      request.setAttribute(attributeName, result)
      result
    }
}