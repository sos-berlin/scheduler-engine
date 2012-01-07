package com.sos.scheduler.engine.plugins.jetty

import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.WebServiceFunctions.{getOrSetAttribute, stripFromEnd}
import javax.ws.rs.WebApplicationException
import javax.ws.rs.core.Response.Status._

@Singleton
class JobLogServlet @Inject()(jobSubsystem: JobSubsystem) extends HttpServlet {
  import JobLogServlet._

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
