package com.sos.scheduler.engine.plugins.jetty.utils

import GetOnlyServlet._
import com.sos.scheduler.engine.common.scalautil.Logger
import javax.servlet.http.{HttpServletRequest, HttpServletResponse, HttpServlet}

/**
 * @author Joacim Zschimmer
 */
trait GetOnlyServlet extends HttpServlet {

  override protected def service(request: HttpServletRequest, response: HttpServletResponse): Unit = {
    request.getMethod match {
      case "GET" | "HEAD" ⇒ super.service(request, response)
      case "OPTIONS" ⇒ response.setHeader("Allow", "OPTIONS,GET,HEAD")
      case method ⇒
        logger.debug(s"HTTP $method not allowed")
        response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED, "")
    }
  }
}

private object GetOnlyServlet {
  private val logger = Logger(getClass)
}
