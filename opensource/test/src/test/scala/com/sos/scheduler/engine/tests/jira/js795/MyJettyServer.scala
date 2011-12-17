package com.sos.scheduler.engine.tests.jira.js795

import org.eclipse.jetty.servlet.{DefaultServlet, ServletContextHandler}
import org.eclipse.jetty.server.Server
import com.sun.jersey.guice.JerseyServletModule;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer
import javax.ws.rs.Path
import com.google.inject.servlet.{GuiceServletContextListener, GuiceFilter}
import com.google.inject.{Guice, Module}

class MyJettyServer(jettyPort: Int, contextPath: String, guiceModule: Module) {
  import MyJettyServer._

  val server = new Server(jettyPort)
  server.setHandler(newContextHandler());

  private def newContextHandler() = {
    val result = new ServletContextHandler(ServletContextHandler.SESSIONS)
    result.setContextPath(contextPath)
    result.addEventListener(new GuiceServletContextListener {
      override def getInjector = Guice.createInjector(
        new JerseyServletModule {
          override protected def configureServlets() {
            install(guiceModule)
            //serve("/command.servlet").`with`(classOf[CommandServlet])
            bind(classOf[EmptyResource]) // Must configure at least one JAX-RS resource or the server will fail to start.
            bind(classOf[CommandResource])
            serve("/*").`with`(classOf[GuiceContainer]) // Route all requests through GuiceContainer
          }
        })
    })

    // Then add GuiceFilter and configure the server to reroute all requests through this filter.
    result.addFilter(classOf[GuiceFilter], "/*", null)

    // Must add DefaultServlet for embedded Jetty. Failing to do this will cause 404 errors. This is not needed if web.xml is used instead.
    result.addServlet(classOf[DefaultServlet], "/")

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