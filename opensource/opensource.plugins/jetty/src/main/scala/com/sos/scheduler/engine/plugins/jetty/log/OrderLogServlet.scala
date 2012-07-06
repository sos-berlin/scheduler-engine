package com.sos.scheduler.engine.plugins.jetty.log

import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}

@Singleton
class OrderLogServlet @Inject()(orderSubsystem: OrderSubsystem) extends HttpServlet {
  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val attributeName = classOf[OrderLogServlet].getName
    Option(request.getAttribute(attributeName).asInstanceOf[FileServletAsyncOperation]) match {
      case None =>
        val jobChainPath = JobChainPath.of(Option(request.getParameter("job_chain")).get)
        val orderIdString = Option(request.getParameter("order")).get
        val jobChain = orderSubsystem.jobChain(jobChainPath)
        val order = jobChain.order(new OrderId(orderIdString))
        val result = LogServletAsyncOperation(request, response, order.getLog)
        request.setAttribute(attributeName, result)
        result
      case Some(operation) =>
        operation.continue()
    }
  }
}

object OrderLogServlet {
  val PathInfoRegex = """order[.]log""".r
}
