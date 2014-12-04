package com.sos.scheduler.engine.plugins.jetty

import com.google.inject.servlet.GuiceFilter
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration.{WarEntry, WebAppContextConfiguration}
import com.sos.scheduler.engine.plugins.jetty.configuration.{Config, JettyConfiguration}
import java.io.File
import javax.servlet.Filter
import org.eclipse.jetty.security._
import org.eclipse.jetty.server._
import org.eclipse.jetty.server.handler._
import org.eclipse.jetty.server.nio.SelectChannelConnector
import org.eclipse.jetty.servlet._
import org.eclipse.jetty.servlets.GzipFilter
import org.eclipse.jetty.util.security.Constraint
import org.eclipse.jetty.webapp.{WebAppContext, WebXmlConfiguration}
import org.eclipse.jetty.xml.XmlConfiguration
import scala.language.reflectiveCalls

object JettyServerBuilder {
  def newJettyServer(config: JettyConfiguration) = {
    def newJobSchedulerContextHandler(): Handler = {
      val webAppContext = config.webAppContextConfigurationOption match {
        case Some(c) ⇒ newWebAppContext(config.contextPath, c)
        case None ⇒ new ServletContextHandler sideEffect { o ⇒ setStandardsIn(o, config.contextPath) }
      }
      for (modify <- config.rootServletContextHandlerModifiers) {
        modify(webAppContext)
      }
      // GuiceFilter (Guice 3.0) kann nur einmal verwendet werden, siehe http://code.google.com/p/google-guice/issues/detail?id=635
      webAppContext.addFilter(classOf[GuiceFilter], "/*", null)  // Reroute all requests through this filter
      if (!servletContextHandlerHasWebXml(webAppContext)) {
        webAppContext.addServlet(classOf[DefaultServlet], "/")   // Failing to do this will cause 404 errors. This is not needed if web.xml is used instead.
      }
      webAppContext
    }

    def newWebAppContext(contextPath: String, c: WebAppContextConfiguration) = {
      val webAppContext = newStandardWebAppContext(contextPath)
      webAppContext.setResourceBase(c.resourceBaseURL.toExternalForm)
      for (f <- c.webXMLFileOption) webAppContext.setDescriptor(f.getPath)
      new WebXmlConfiguration().configure(webAppContext)
      webAppContext
    }

    def newWarWebAppContext(contextPath: String, warFile: File): WebAppContext = {
      val webAppContext = newStandardWebAppContext(contextPath)
      webAppContext.setWar(warFile.getAbsolutePath)
      new WebXmlConfiguration().configure(webAppContext)
      webAppContext
    }

    def newStandardWebAppContext(contextPath: String): WebAppContext = {
      val webAppContext = new WebAppContext
      webAppContext.setThrowUnavailableOnStartupException(true)
      setStandardsIn(webAppContext, contextPath = contextPath)
      webAppContext
    }

    def newRootContextHandler() = {
      val result = new ServletContextHandler(ServletContextHandler.SESSIONS)
      result.setContextPath("/")
      result.addFilter(classOf[VerbRestrictionFilter], "/*", null)
      for (modify <- config.rootServletContextHandlerModifiers) {
        modify(result)
      }
      result.addServlet(classOf[PingServlet], "/PING")
      result
    }

    def setStandardsIn(handler: ServletContextHandler, contextPath: String): Unit = {
      handler.setInitParameter("org.eclipse.jetty.servlet.Default.dirAllowed", "false")
      handler.setContextPath(contextPath)
      for (s <- config.loginServiceOption) {
        handler.setSecurityHandler(newConstraintSecurityHandler(s, List(Config.administratorRoleName)))
      }
      handler.addFilter(classOf[VerbRestrictionFilter], "/*", null)
      def addFilter[F <: Filter](filter: Class[F], path: String, initParameters: (String, String)*): Unit = {
        handler.getServletHandler.addFilterWithMapping(newFilterHolder(filter, initParameters), path, null)
      }
      addFilter(classOf[GzipFilter], "/*") //, "mimeTypes" -> gzipContentTypes.mkString(","))
    }

    val result = new Server
    for (o <- config.portOption) result.addConnector(newConnector(o.value))
    val logHandlerOption = config.accessLogFileOption map { o ⇒ newRequestLogHandler(new NCSARequestLog(o.getPath)) }
    result.setHandler(newHandlerCollection(
      logHandlerOption ++
      Some(newJobSchedulerContextHandler()) ++
      Some(newRootContextHandler()) ++
      (config.wars map { case WarEntry(contextPath, warFile) ⇒ newWarWebAppContext(contextPath, warFile) }) ++
      Some(new RootForwardingHandler) ++
      Some(new DefaultHandler)))
    for (o <- config.jettyXMLURLOption) new XmlConfiguration(o).configure(result)
    result
  }

  private def newConnector(port: Int) = {
    val connector = new SelectChannelConnector
    connector.setPort(port)
    connector
  }

  private def newRequestLogHandler(r: RequestLog) = {
    val result = new RequestLogHandler
    result.setRequestLog(r)
    result
  }

  private def newHandlerCollection(handlers: Iterable[Handler]) = {
    val result = new HandlerCollection
    result.setHandlers(handlers.toArray)
    result
  }

  private def servletContextHandlerHasWebXml(h: ServletContextHandler) = h match {
    case w: WebAppContext ⇒ w.getWebInf != null
    case _ ⇒ false
  }

  private def newFilterHolder[F <: Filter](c: Class[F], initParameters: Iterable[(String, String)]) = {
    val result = new FilterHolder(Holder.Source.EMBEDDED)
    result.setHeldClass(c)
    for (p <- initParameters) result.setInitParameter(p._1, p._2)
    result
  }

  private def newConstraintSecurityHandler(loginService: LoginService, roles: Iterable[String]) = {
    val constraint = {
      val o = new Constraint()
      o.setName(Constraint.__BASIC_AUTH)
      o.setRoles(roles.toArray)
      o.setAuthenticate(true)
      o
    }
    val constraintMapping = {
      val o = new ConstraintMapping()
      o.setConstraint(constraint)
      o.setPathSpec("/*")
      o
    }
    val result = new ConstraintSecurityHandler
    result.setRealmName(loginService.getName)
    result.setLoginService(loginService)
    result.setConstraintMappings(Array(constraintMapping))
    result
  }
}
