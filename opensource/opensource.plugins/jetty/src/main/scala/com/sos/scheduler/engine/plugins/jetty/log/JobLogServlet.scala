package com.sos.scheduler.engine.plugins.jetty.log

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.utils.GetOnlyServlet
import com.sos.scheduler.engine.plugins.jetty.utils.Utils.getOrSetAttribute
import javax.inject.{Inject, Singleton}
import javax.servlet.http.{HttpServletRequest, HttpServletResponse}

@Singleton
final class JobLogServlet @Inject private(jobSubsystem: JobSubsystem) extends GetOnlyServlet {

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[JobLogServlet].getName) {
      val job = jobSubsystem.job(JobPath.makeAbsolute(Option(request.getParameter("job")).get))
      LogServletAsyncOperation(request, response, job.log)
    }
    operation.continue()
  }
}
