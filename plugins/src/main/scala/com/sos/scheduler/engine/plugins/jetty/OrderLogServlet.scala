package com.sos.scheduler.engine.plugins.jetty

import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import javax.ws.rs.WebApplicationException
import javax.ws.rs.core.Response.Status.BAD_REQUEST
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.order.{OrderId, OrderSubsystem}
import com.sos.scheduler.engine.plugins.jetty.WebServiceFunctions._

@Singleton
class OrderLogServlet @Inject()(orderSubsystem: OrderSubsystem) extends HttpServlet {
  import OrderLogServlet._

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val attributeName = classOf[OrderLogServlet].getName
    Option(request.getAttribute(attributeName).asInstanceOf[FileServletAsyncOperation]) match {
      case None =>
        val jobChainPathString = Option(request.getParameter("job_chain")).get
        val orderIdString = Option(request.getParameter("order")).get
        val jobChain = orderSubsystem.jobChain(AbsolutePath.of(jobChainPathString))
        val order = jobChain.order(new OrderId(orderIdString))
        val result = LogServletAsyncOperation(request, response, order.getLog)
        request.setAttribute(attributeName, result)
        result
      case Some(operation) =>
        operation.continue()
    }
//    val operation = getOrSetAttribute(request, classOf[OrderLogServlet].getName) {
//      request.getPathInfo match {
//        case PathInfoRegex(jobChainPathString, orderIdString) =>
//          val jobChain = orderSubsystem.jobChain(AbsolutePath.of(jobChainPathString))
//          val order = jobChain.order(new OrderId(orderIdString))
//          LogServletAsyncOperation(request, response, order.getLog)
//        case _ =>
//          throw new WebApplicationException(BAD_REQUEST)
//      }
//    }
//    operation.continue()
  }
}

object OrderLogServlet {
  val PathInfoRegex = """order[.]log""".r
}
