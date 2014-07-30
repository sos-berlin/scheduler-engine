package com.sos.scheduler.engine.testwar

import java.io.OutputStreamWriter
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

/**
 * @author Joacim Zschimmer
 */
final class TestServlet extends HttpServlet {

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    response.setContentType("text/plain")
    val writer = new OutputStreamWriter(response.getOutputStream)
    writer.write("TestServlet")
    writer.flush()
  }
}
