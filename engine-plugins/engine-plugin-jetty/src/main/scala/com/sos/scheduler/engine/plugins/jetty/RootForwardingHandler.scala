package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.plugins.jetty.configuration.Config.contextPath
import com.sos.scheduler.engine.plugins.jetty.utils.GetOnlyServlet.withGetOrHeadOnly
import javax.servlet.http.HttpServletResponse.SC_TEMPORARY_REDIRECT
import javax.servlet.http.{HttpServletRequest, HttpServletResponse}
import org.eclipse.jetty.server.Request
import org.eclipse.jetty.server.handler.AbstractHandler

final class RootForwardingHandler extends AbstractHandler {

  def handle(target: String, baseRequest: Request, request: HttpServletRequest, response: HttpServletResponse) = {
    if (!baseRequest.isHandled && !response.isCommitted && (Set("/", "/jobscheduler", "/jobscheduler/") contains request.getRequestURI)) {
      baseRequest.setHandled(true)
      withGetOrHeadOnly(request, response) {
        response.setStatus(SC_TEMPORARY_REDIRECT)
        response.setHeader("Location", s"$contextPath/operations_gui/")
      }
    }
  }
}
