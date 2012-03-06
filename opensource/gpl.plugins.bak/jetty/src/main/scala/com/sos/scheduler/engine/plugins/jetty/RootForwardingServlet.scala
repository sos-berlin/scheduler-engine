package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.plugins.jetty.Config.contextPath
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import javax.servlet.http.HttpServletResponse.SC_MOVED_TEMPORARILY

class RootForwardingServlet extends HttpServlet {
  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
      response.setStatus(SC_MOVED_TEMPORARILY)
      response.setHeader("Location", contextPath+request.getRequestURI)
  }
}
