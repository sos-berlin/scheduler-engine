package com.sos.scheduler.engine.plugins.jetty.log

import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import com.sos.scheduler.engine.data.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices.getOrSetAttribute

@Singleton
class JobLogServlet @Inject()(jobSubsystem: JobSubsystem) extends HttpServlet {
  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[JobLogServlet].getName) {
      val jobPathString = Option(request.getParameter("job")).get
      val job = jobSubsystem.job(AbsolutePath.of(jobPathString))
      LogServletAsyncOperation(request, response, job.getLog)
    }
    operation.continue()
  }
}

object JobLogServlet {
  val PathInfoRegex = """job[.]log""".r
}
