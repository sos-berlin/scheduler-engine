package com.sos.scheduler.engine.plugins.jetty.log

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.utils.Utils.getOrSetAttribute
import javax.inject.{Inject, Singleton}
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

@Singleton
final class JobLogServlet @Inject private(jobSubsystem: JobSubsystem) extends HttpServlet {

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[JobLogServlet].getName) {
      val job = jobSubsystem.job(JobPath.makeAbsolute(Option(request.getParameter("job")).get))
      LogServletAsyncOperation(request, response, job.log)
    }
    operation.continue()
  }
}

object JobLogServlet {
  val PathInfoRegex = """job[.]log""".r
}
