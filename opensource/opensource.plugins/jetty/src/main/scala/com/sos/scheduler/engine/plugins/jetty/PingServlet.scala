package com.sos.scheduler.engine.plugins.jetty

import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

final class PingServlet extends HttpServlet {

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    response.setContentType("text/plain")
    response.getOutputStream.print("PONG")
  }
}
