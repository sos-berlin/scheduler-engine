package com.sos.scheduler.engine.plugins.jetty

import com.google.inject.Guice.createInjector
import com.google.inject.Injector
import com.google.inject.servlet.{GuiceFilter, GuiceServletContextListener}
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerConfiguration, HasGuiceModule}
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin
import com.sos.scheduler.engine.kernel.util.XmlUtils.childElementOrNull
import com.sos.scheduler.engine.plugins.jetty.Config._
import java.net.{URL, ServerSocket, BindException}
import javax.inject.Inject
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
import org.slf4j.LoggerFactory
import org.w3c.dom.Element

/** JS-795: Einbau von Jetty in den JobScheduler. */
final class JettyPlugin @Inject()(pluginElement: Element, hasGuiceModule: HasGuiceModule, schedulerConfiguration: SchedulerConfiguration)
  extends AbstractPlugin {

  import JettyPlugin._

  private val config = new Config(pluginElement, schedulerConfiguration)
  private var started = false

  /** Der Port des ersten Connector */
  def tcpPortNumber = server.getConnectors.head.getPort

  private val server = {
    val schedulerModule = hasGuiceModule.getGuiceModule
    val loginServiceOption = childElementOption(pluginElement, "loginService") map PluginLoginService.apply
    val myInjector = createInjector(schedulerModule, Config.newServletModule())
    val contextHandler = jobSchedulerContextHandler(contextPath, myInjector, loginServiceOption)
    newServer(
      config.tryUntilPortOption map { until => findFreePort(config.portOption.get, until) } orElse config.portOption,
      config.jettyXmlFileOption map { f => new XmlConfiguration(f.toURI.toURL) },
      newHandlerCollection(Iterable(
        newRequestLogHandler(new NCSARequestLog(config.accessLogFile.toString)),
        new StatisticsHandler,
        contextHandler,
        newRootContextHandler(),
        new DefaultHandler())))
  }

  private def jobSchedulerContextHandler(contextPath: String, injector: Injector, loginService: Option[LoginService]) = {
    val result = newWebAppContext(resourceBaseURL)

    def addFilter[F <: Filter](filter: Class[F], path: String, initParameters: (String, String)*) {
      result.getServletHandler.addFilterWithMapping(newFilterHolder(filter, initParameters), path, null)
    }

    result.setContextPath(contextPath)
    result.addEventListener(new GuiceServletContextListener { def getInjector = injector })
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

  override def activate() {
    server.start()
    started = true
  }

  override def close() {
    server.stop()
    if (started) {
      server.join()
      started = false
    }
  }
}

object JettyPlugin {
  private val logger = LoggerFactory.getLogger(classOf[JettyPlugin])

  private def newServer(port: Option[Int], configuration: Option[XmlConfiguration], handler: Handler, beans: Iterable[AnyRef] = Iterable()) = {
    val result = new Server
    for (c <- configuration) c.configure(result)
    for (p <- port) result.addConnector(newConnector(p))
    result.setHandler(newHandlerCollection(Option(result.getHandler) ++ Iterable(handler)))
    for (b <- beans) result.addBean(b)
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

  def findFreePort(firstPort: Int, end: Int): Int =
    if (firstPort >= end) firstPort
    else {
      try {
        val backlog = 1
        new ServerSocket(firstPort, backlog).close()
        firstPort
      } catch {
        case _: BindException => findFreePort(firstPort + 1, end)
      }
  }

  private def childElementOption(e: Element, name: String) = Option(childElementOrNull(e, name))
}
// jobscheduler/engine/  Übersicht über den Zustand und weitere URIs
// jobscheduler/engine/log  Hauptprotokoll
// jobscheduler/engine/log.snapshot  Hauptprotokoll
// jobscheduler/engine/configuration
// jobscheduler/engine/folders?folder=PATH?type={folder,job,jobChain,...}&deep={false,true}
// jobscheduler/engine/{folders,jobs,jobChains}?folder=PATH&deep={false,true}
// jobscheduler/engine/{folder,job,jobChain}?job=PATH  Übersicht über alle Informationen zum Objekt
// jobscheduler/engine/tasks => Liste der Tasks
// jobscheduler/engine/task/TASKID/state
// jobscheduler/engine/task/TASKID/log
// jobscheduler/engine/task/TASKID/log.state
// jobscheduler/engine/order?jobChain=PATH&order=ORDERID
// jobscheduler/engine/order/configuration?jobChain=PATH&order=ORDERID
// jobscheduler/engine/order/log?jobChain=PATH&order=ORDERID&historyId=...
// jobscheduler/engine/order/log?jobChain=PATH&order=ORDERID
// jobscheduler/engine/order/log.snapshot?jobChain=PATH&order=ORDERID
// jobscheduler/engine/job_chain/configuration?jobChain=PATH
// jobscheduler/engine/job_chain/state?jobChain=PATH
// / -> jobscheduler/ -> jobscheduler/engine/

// ERLEDIGT:
// jobscheduler/engine/gui/  Operations GUI, wenn über web.xml konfiguriert
// jobscheduler/engine/z/  Alte GUI
// jobscheduler/engine/command&command=XMLCOMMAND
// jobscheduler/engine/command  POST
// jobscheduler/engine/job/configuration?job=PATH => Job-Konfiguration
// jobscheduler/engine/job/description?job=PATH
// jobscheduler/engine/job/log.snapshot?job=PATH
