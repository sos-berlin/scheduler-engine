package com.sos.scheduler.engine.plugins.jetty

import com.google.inject.servlet.GuiceFilter
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration.WebAppContextConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.{JettyConfiguration, Config}
import javax.servlet.Filter
import org.eclipse.jetty.security._
import org.eclipse.jetty.server._
import org.eclipse.jetty.server.handler._
import org.eclipse.jetty.server.nio.SelectChannelConnector
import org.eclipse.jetty.servlet._
import org.eclipse.jetty.servlets.GzipFilter
import org.eclipse.jetty.util.security.Constraint
import org.eclipse.jetty.webapp.{WebXmlConfiguration, WebAppContext}
import org.eclipse.jetty.xml.XmlConfiguration
import scala.language.reflectiveCalls

object JettyServerBuilder {
  def newJettyServer(config: JettyConfiguration) = {
    def newContextHandler(): Handler = {
      val webAppContext = config.webAppContextConfigurationOption match {
        case Some(c) => newWebAppContext(c)
        case None => new ServletContextHandler
      }
      (webAppContext: { def setContextPath(p: String) }).setContextPath(config.contextPath)
      webAppContext.addFilter(classOf[VerbRestrictionFilter], "/*", null)   // This is redundant security. Jetty itself seams to filter TRACE
      for (modify <- config.servletContextHandlerModifiers) {
        modify(webAppContext)
      }
      if (config.gzip) {
        def addFilter[F <: Filter](filter: Class[F], path: String, initParameters: (String, String)*) {
          webAppContext.getServletHandler.addFilterWithMapping(newFilterHolder(filter, initParameters), path, null)
        }
        addFilter(classOf[GzipFilter], "/*") //, "mimeTypes" -> gzipContentTypes.mkString(","))
      }
      // GuiceFilter (Guice 3.0) kann nur einmal verwendet werden, siehe http://code.google.com/p/google-guice/issues/detail?id=635
      webAppContext.addFilter(classOf[GuiceFilter], "/*", null)  // Reroute all requests through this filter
      if (!servletContextHandlerHasWebXml(webAppContext)) {
        webAppContext.addServlet(classOf[DefaultServlet], "/")   // Failing to do this will cause 404 errors. This is not needed if web.xml is used instead.
        //TODO init-parameter dirAllowed=false
      }
      for (s <- config.loginServiceOption) webAppContext.setSecurityHandler(newConstraintSecurityHandler(s, List(Config.administratorRoleName)))
      webAppContext
    }

    def newWebAppContext(c: WebAppContextConfiguration) = {
      val webAppContext = new WebAppContext
      webAppContext.setThrowUnavailableOnStartupException(true)
      webAppContext.setResourceBase(c.resourceBaseURL.toExternalForm)
      for (f <- c.webXMLFileOption) webAppContext.setDescriptor(f.getPath)
      new WebXmlConfiguration().configure(webAppContext)
      webAppContext
    }

    val result = new Server
    for (o <- config.portOption) result.addConnector(newConnector(o.value))
    val logHandlerOption = config.accessLogFileOption map { o => newRequestLogHandler(new NCSARequestLog(o.getPath)) }
    result.setHandler(newHandlerCollection(
      logHandlerOption ++
      Some(newContextHandler()) ++
      config.handlers ++
      Some(new DefaultHandler)))
    for (o <- config.jettyXMLURLOption) new XmlConfiguration(o).configure(result)
    result
  }

  def newConnector(port: Int) = {
    val connector = new SelectChannelConnector
    connector.setPort(port)
    connector
  }

  def newRequestLogHandler(r: RequestLog) = {
    val result = new RequestLogHandler
    result.setRequestLog(r)
    result
  }

  def newHandlerCollection(handlers: Iterable[Handler]) = {
    val result = new HandlerCollection
    result.setHandlers(handlers.toArray)
    result
  }

  def servletContextHandlerHasWebXml(h: ServletContextHandler) = h match {
    case w: WebAppContext => w.getWebInf != null
    case _ => false
  }

  def newFilterHolder[F <: Filter](c: Class[F], initParameters: Iterable[(String, String)]) = {
    val result = new FilterHolder(Holder.Source.EMBEDDED)
    result.setHeldClass(c)
    for (p <- initParameters) result.setInitParameter(p._1, p._2)
    result
  }

  def newConstraintSecurityHandler(loginService: LoginService, roles: Iterable[String]) = {
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
