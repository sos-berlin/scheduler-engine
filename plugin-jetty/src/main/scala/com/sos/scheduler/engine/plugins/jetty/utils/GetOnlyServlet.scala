package com.sos.scheduler.engine.plugins.jetty.utils

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.plugins.jetty.utils.GetOnlyServlet._
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

/**
 * @author Joacim Zschimmer
 */
trait GetOnlyServlet extends HttpServlet {

  override protected final def service(request: HttpServletRequest, response: HttpServletResponse): Unit =
    withGetOrHeadOnly(request, response) {
      super.service(request, response)
    }
}

object GetOnlyServlet {
  private val logger = Logger(getClass)

  def withGetOrHeadOnly(request: HttpServletRequest, response: HttpServletResponse)(body: => Unit): Unit =
    request.getMethod match {
      case "GET" | "HEAD" ⇒ body
      case "OPTIONS" ⇒ response.setHeader("Allow", "OPTIONS,GET,HEAD")
      case method ⇒
        logger.debug(s"HTTP $method not allowed")
        response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED, "")
    }
}
