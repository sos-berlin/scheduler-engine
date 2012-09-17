package com.sos.scheduler.engine.plugins.jetty.log

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices.getOrSetAttribute
import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

@Singleton
class JobLogServlet @Inject()(jobSubsystem: JobSubsystem) extends HttpServlet {
  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[JobLogServlet].getName) {
      val job = jobSubsystem.job(JobPath.of(Option(request.getParameter("job")).get))
      LogServletAsyncOperation(request, response, job.getLog)
    }
    operation.continue()
  }
}

object JobLogServlet {
  val PathInfoRegex = """job[.]log""".r
}
