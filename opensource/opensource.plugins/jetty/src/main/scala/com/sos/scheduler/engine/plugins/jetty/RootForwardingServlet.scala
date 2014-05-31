package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.plugins.jetty.configuration.Config.contextPath
import com.sos.scheduler.engine.plugins.jetty.utils.GetOnlyServlet
import javax.servlet.http.HttpServletResponse.SC_MOVED_TEMPORARILY
import javax.servlet.http.{HttpServletRequest, HttpServletResponse}

class RootForwardingServlet extends GetOnlyServlet {
  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
      response.setStatus(SC_MOVED_TEMPORARILY)
      response.setHeader("Location", contextPath+request.getRequestURI)
  }
}
