package com.sos.scheduler.engine.plugins.jetty.log

import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices.getOrSetAttribute
import com.sos.scheduler.engine.kernel.Scheduler

@Singleton
class MainLogServlet @Inject()(scheduler: Scheduler) extends HttpServlet {
  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[MainLogServlet].getName) {
      LogServletAsyncOperation(request, response, scheduler.log)
    }
    operation.continue()
  }
}
