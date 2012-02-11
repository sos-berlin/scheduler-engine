package com.sos.scheduler.engine.plugins.jetty.cpp

import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import javax.servlet.http.HttpServletResponse.SC_MOVED_PERMANENTLY
import org.slf4j.LoggerFactory
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxyInvalidatedException, DisposableCppProxyRegister}
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerIsClosed, SchedulerHttpService}

@Singleton
class CppServlet @Inject()(
    schedulerHttpService: SchedulerHttpService,
    cppProxyRegister: DisposableCppProxyRegister,
    schedulerIsClosed: SchedulerIsClosed)
    extends HttpServlet {

  import CppServlet._

  override def service(request: HttpServletRequest, response: HttpServletResponse) {
    if (request.getMethod == "GET" && request.getPathInfo == null) {
      response.setStatus(SC_MOVED_PERMANENTLY)
      response.setHeader("Location", request.getRequestURI +"/") // Basis muss mit Schrägstrich enden
    } else
      normalService(request, response)
  }

  def normalService(request: HttpServletRequest, response: HttpServletResponse) {
    val attributeName = classOf[CppServlet].getName

    def startOperation() {
      val operation = new Operation(request, response, schedulerHttpService, cppProxyRegister, schedulerIsClosed)
      try operation.start()
      catch { case x => operation.tryClose(); throw x }
      if (!operation.isClosed) request.setAttribute(attributeName, operation)
    }

    def continueOperation(operation: Operation) {
      try operation.continue()
      catch { case x => operation.tryClose(); throw x }
      finally if (operation.isClosed) request.removeAttribute(attributeName)
    }

    try request.getAttribute(attributeName) match {
      case null => startOperation()
      case o: Operation => continueOperation(o)
    }
    catch {
      case x: InterruptedException => logger.debug(x.toString)
      case x: RuntimeException if x.getCause.isInstanceOf[InterruptedException] => logger.debug(x.toString)
      case x: CppProxyInvalidatedException => logger.debug(x.toString)
    }
  }
}

object CppServlet {
  private val logger = LoggerFactory.getLogger(classOf[CppServlet])
}
