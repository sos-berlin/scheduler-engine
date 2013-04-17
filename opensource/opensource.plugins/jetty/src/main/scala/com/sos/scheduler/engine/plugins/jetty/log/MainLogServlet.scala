package com.sos.scheduler.engine.plugins.jetty.log

import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.getOrSetAttribute
import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

@Singleton
class MainLogServlet @Inject()(prefixLog: PrefixLog) extends HttpServlet {

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[MainLogServlet].getName) {
      LogServletAsyncOperation(request, response, prefixLog)
    }
    operation.continue()
  }
}
