package com.sos.scheduler.engine.plugins.jetty

import ServerBuilder._
import com.google.inject.servlet.GuiceFilter
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.common.xml.XmlUtils.childElementOrNull
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.Config._
import java.net.URL
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
import org.w3c.dom.Element

class ServerBuilder(pluginElement: Element, schedulerConfiguration: SchedulerConfiguration) {

  private val config = new Config(pluginElement, schedulerConfiguration)

  private val server = {
    val loginServiceOption = childElementOption(pluginElement, "loginService") map PluginLoginService.apply
    val contextHandler = jobSchedulerContextHandler(contextPath, loginServiceOption)
    newServer(
      config.tryUntilPortOption map { until => findRandomFreePort(config.portOption.get until until) } orElse config.portOption,
      config.jettyXmlFileOption map { f => new XmlConfiguration(f.toURI.toURL) },
      newHandlerCollection(Iterable(
        newRequestLogHandler(new NCSARequestLog(config.accessLogFile.toString)),
        new StatisticsHandler,
        contextHandler,
        newRootContextHandler(),
        new DefaultHandler())))
  }

  private def jobSchedulerContextHandler(contextPath: String, loginService: Option[LoginService]) = {
    val result = newWebAppContext(resourceBaseURL)

    def addFilter[F <: Filter](filter: Class[F], path: String, initParameters: (String, String)*) {
      result.getServletHandler.addFilterWithMapping(newFilterHolder(filter, initParameters), path, null)
    }

    result.setContextPath(contextPath)
    //result.addEventListener(new GuiceServletContextListener { def getInjector = injector })
    addFilter(classOf[GzipFilter], "/*") //, "mimeTypes" -> gzipContentTypes.mkString(","))
    // GuiceFilter (Guice 3.0) kann nur einmal verwendet werden, siehe http://code.google.com/p/google-guice/issues/detail?id=635
    result.addFilter(classOf[GuiceFilter], "/*", null)  // Reroute all requests through this filter
    if (!servletContextHandlerHasWebXml(result)) {
      logger.debug("No web.xml, adding DefaultServlet")
      result.addServlet(classOf[DefaultServlet], "/")   // Failing to do this will cause 404 errors. This is not needed if web.xml is used instead.
      //TODO init-parameter dirAllowed=false
    }
    for (s <- loginService)  result.setSecurityHandler(newConstraintSecurityHandler(s))
    result
  }

  private def newWebAppContext(baseUrl: URL) = {
    val result = new WebAppContext
    result.setResourceBase(baseUrl.toString)
    for (f <- config.webXmlFileOption)  result.setDescriptor(f.getPath)
    new WebXmlConfiguration().configure(result)
    result
  }

  private def newRootContextHandler() = {
    val result = new ServletContextHandler(ServletContextHandler.SESSIONS)
    result.setContextPath("/")
    result.addServlet(classOf[RootForwardingServlet], "/")
    result
  }

  def build(): Server = server
}

object ServerBuilder {
  private val logger = Logger(getClass)

  private def newServer(port: Option[Int], configuration: Option[XmlConfiguration], handler: Handler, beans: Iterable[AnyRef] = Iterable()) = {
    val result = new Server
    for (p <- port) result.addConnector(newConnector(p))
    result.setHandler(newHandlerCollection(Iterable(handler)))
    for (b <- beans) result.addBean(b)
    for (c <- configuration) c.configure(result)
    result
  }

  private def newConnector(port: Int) = {
    val connector = new SelectChannelConnector
    connector.setPort(port)
    connector
  }

  private def newHandlerCollection(handlers: Iterable[Handler]) = {
    val result = new HandlerCollection
    result.setHandlers(handlers.toArray)
    result
  }

  private def newRequestLogHandler(r: RequestLog) = {
    val result = new RequestLogHandler
    result.setRequestLog(r)
    result
  }

  private def servletContextHandlerHasWebXml(h: ServletContextHandler) = h match {
    case w: WebAppContext => w.getWebInf != null
    case _ => false
  }

  private def newFilterHolder[F <: Filter](c: Class[F], initParameters: Iterable[(String, String)]) = {
    val result = new FilterHolder(Holder.Source.EMBEDDED)
    result.setHeldClass(classOf[GzipFilter])
    for (p <- initParameters) result.setInitParameter(p._1, p._2)
    result
  }

  private def newConstraintSecurityHandler(loginService: LoginService) = {
    val constraint = {
      val o = new Constraint()
      o.setName(Constraint.__BASIC_AUTH)
      o.setRoles(Array(adminstratorRoleName))
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

  private def childElementOption(e: Element, name: String) =
    Option(childElementOrNull(e, name))
}
