package com.sos.scheduler.engine.plugins.jetty

import javax.ws.rs.Path
import com.google.inject.servlet.{GuiceServletContextListener, GuiceFilter}
import org.eclipse.jetty.server.Server
import org.eclipse.jetty.servlet.{DefaultServlet, ServletContextHandler}
import com.sos.scheduler.engine.kernel.scheduler.GetGuiceModule
import com.google.inject.{Module, Guice, Injector}

class MyJettyServer(jettyPort: Int, contextPath: String, injector: Injector, servletModule: Module) {
  import MyJettyServer._

  val server = {
    val servletContextListener = new GuiceServletContextListener {
      override def getInjector =
        //Funktioniert nicht: injector.createChildInjector(servletModule)
        Guice.createInjector(servletModule, injector.getInstance(classOf[GetGuiceModule]).getGuiceModule)
    }
    val contextHandler = {
      val o = new ServletContextHandler(ServletContextHandler.SESSIONS)
      o.setContextPath(contextPath)
      o.addEventListener(servletContextListener)
      o.addFilter(classOf[GuiceFilter], "/*", null)  // Reroute all requests through this filter
      o.addServlet(classOf[DefaultServlet], "/")   // Failing to do this will cause 404 errors. This is not needed if web.xml is used instead.
      o
    }
    val result = new Server(jettyPort)
    result.setHandler(contextHandler)
    result
  }

  def start() {
    server.start()
  }

  def stop() {
    server.stop()
  }

  def close() {
    server.stop()
    server.join()
  }
}

object MyJettyServer {
  @Path("--empty--")
  class EmptyResource
}
