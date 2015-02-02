package com.sos.scheduler.engine.plugins.jetty.log

import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.jetty.utils.GetOnlyServlet
import com.sos.scheduler.engine.plugins.jetty.utils.Utils.getOrSetAttribute
import javax.inject.{Inject, Singleton}
import javax.servlet.http.{HttpServletRequest, HttpServletResponse}

@Singleton
final class MainLogServlet @Inject private(prefixLog: PrefixLog) extends GetOnlyServlet {

  override def doGet(request: HttpServletRequest, response: HttpServletResponse): Unit = {
    val operation = getOrSetAttribute(request, classOf[MainLogServlet].getName) {
      LogServletAsyncOperation(request, response, prefixLog)
    }
    operation.continue()
  }
}
