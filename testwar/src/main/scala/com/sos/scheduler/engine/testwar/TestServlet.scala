package com.sos.scheduler.engine.testwar

import com.sos.scheduler.engine.common.scalautil.HasCloser
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

/**
 * @author Joacim Zschimmer
 */
final class TestServlet extends HttpServlet
with HasCloser {   // Refers to external jar in parent classpath

  override def doGet(request: HttpServletRequest, response: HttpServletResponse): Unit = {
    response.setContentType("text/plain")
    response.getOutputStream.print("TestServlet")
  }
}
