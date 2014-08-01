package com.sos.scheduler.engine.testwar

import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

/**
 * @author Joacim Zschimmer
 */
final class TestServlet extends HttpServlet {

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    response.setContentType("text/plain")
    response.getOutputStream.print("TestServlet")
  }
}
