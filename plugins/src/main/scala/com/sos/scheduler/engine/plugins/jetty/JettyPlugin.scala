package com.sos.scheduler.engine.plugins.jetty

import com.google.inject.{Injector, Guice}
import com.google.inject.servlet.{GuiceFilter, GuiceServletContextListener}
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerConfiguration, HasGuiceModule}
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin
import com.sos.scheduler.engine.kernel.util.XmlUtils.childElementOrNull
import com.sos.scheduler.engine.plugins.jetty.JettyPluginConfiguration._
import com.sos.scheduler.engine.plugins.jetty.bodywriters.XmlElemWriter
import com.sun.jersey.guice.JerseyServletModule
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer
import javax.inject.Inject
import javax.servlet.Filter
import org.apache.log4j.Logger
import org.eclipse.jetty.security._
import org.eclipse.jetty.servlet.{Holder, FilterHolder, DefaultServlet, ServletContextHandler}
import org.eclipse.jetty.servlets.GzipFilter
import org.eclipse.jetty.server.handler.{RequestLogHandler, HandlerCollection}
import org.eclipse.jetty.server._
import org.eclipse.jetty.util.security.Constraint
import org.eclipse.jetty.server.nio.SelectChannelConnector
import org.eclipse.jetty.xml.XmlConfiguration
import org.w3c.dom.Element
import java.net.{ServerSocket, BindException}

/** JS-795: Einbau von Jetty in den JobScheduler. */
final class JettyPlugin @Inject()(pluginElement: Element, hasGuiceModule: HasGuiceModule, configuration: SchedulerConfiguration)
  extends AbstractPlugin {

  import JettyPlugin._

  private val config = new Config(pluginElement, configuration)

  /** Der Port des ersten Connector */
  def port = server.getConnectors.iterator.next().getPort

  private val server = {
//    val contexts = new ContextHandlerCollection()
//    contexts.setHandlers(Array(
//      newContextHandler(contextPath, Guice.createInjector(schedulerModule, newServletModule())),
//      newContextHandler(cppContextPath, Guice.createInjector(schedulerModule, newCppServletModule()))
//      //Funktioniert nicht: injector.createChildInjector(newServletModule())
//    ))
    newServer(
      port = config.tryUntilPortOption map { until => findFreePort(config.portOption getOrElse until, until) } orElse config.portOption,
      configuration = config.jettyXmlFileOption map { f => new XmlConfiguration(f.toURI.toURL) },
      handlers = List(
        newRequestLogHandler(new NCSARequestLog(config.accessLogFile.toString)),
        newContextHandler(contextPath,
          Guice.createInjector(hasGuiceModule.getGuiceModule, newServletModule()),
          childElementOption(pluginElement, "loginService") map PluginLoginService.apply))
    )
  }

  override def activate() {
    server.start()
  }

  override def close() {
    server.stop()
    server.join()
  }
}

object JettyPlugin {
  private val logger = Logger.getLogger(classOf[JettyPlugin])
  private val contextPath = ""  // Mehrere Kontexte funktionieren nicht und GuiceFilter meldet einen Konflikt. Deshalb simulieren wir mit prefixPath.

  private def newServer(port: Option[Int], handlers: Iterable[Handler], configuration: Option[XmlConfiguration], beans: Iterable[AnyRef] = Iterable()) = {
    val result = new Server
    for (c <- configuration) c.configure(result)
    for (p <- port) result.addConnector(newConnector(p))
    result.setHandler(newHandlerCollection(handlers))
    for (bean <- beans) result.addBean(bean)
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

  private def newContextHandler(contextPath: String, injector: Injector, loginService: Option[LoginService]) = {
    val result = new ServletContextHandler(ServletContextHandler.SESSIONS)

    def addFilter[F <: Filter](filter: Class[F], path: String, initParameters: (String, String)*) {
      result.getServletHandler.addFilterWithMapping(newFilterHolder(filter, initParameters), path, null)
    }

    result.setContextPath(contextPath)
    result.addEventListener(new GuiceServletContextListener { def getInjector = injector })
    addFilter(classOf[GzipFilter], "/*") //, "mimeTypes" -> gzipContentTypes.mkString(","))
    //result.addFilter(classOf[GzipFilter], "/*", null);
    result.addFilter(classOf[GuiceFilter], "/*", null)  // Reroute all requests through this filter
    result.addServlet(classOf[DefaultServlet], "/")   // Failing to do this will cause 404 errors. This is not needed if web.xml is used instead.
    for (s <- loginService)  result.setSecurityHandler(newConstraintSecurityHandler(s))
    result
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

//  private def newCppServletModule() = new JerseyServletModule {
//    override def configureServlets() {
//      serve("/*").`with`(classOf[CppServlet])
//    }
//  }

  private def newServletModule() = new JerseyServletModule {
    override def configureServlets() {
      serveRegex(prefixPath+"/objects/"+JobLogServlet.PathInfoRegex).`with`(classOf[JobLogServlet])
      serveRegex(prefixPath+"/objects/"+OrderLogServlet.PathInfoRegex).`with`(classOf[OrderLogServlet])
      serveRegex(prefixPath+"/log").`with`(classOf[MainLogServlet])
      serve(prefixPath+"/*").`with`(classOf[GuiceContainer]) // Route all requests through GuiceContainer
      bind(classOf[CommandResource])
      bind(classOf[JobResource])
      bind(classOf[JobsResource])
      bind(classOf[XmlElemWriter])
      serve(cppPrefixPath).`with`(classOf[CppServlet])
      serve(cppPrefixPath+"/*").`with`(classOf[CppServlet])
    }
  }

  def findFreePort(begin: Int, end: Int): Int = if (begin >= end) begin else
    try {
      val backlog = 1
      new ServerSocket(begin, backlog).close()
      begin
    } catch {
      case _: BindException => findFreePort(begin + 1, end)
    }

  private def childElementOption(e: Element, name: String) = Option(childElementOrNull(e, name))



  // TODO URIs und REST
  // Alle REST-Aufrufe liefern XML oder JSON.

  // KONTEXTE
  // /JobScheduler/engine/
  // /JobScheduler/gui/

  // JobScheduler/engine/objects/folders/PATH" liefert Ordner
  // JobScheduler/engine/objects/jobs//PATH
  // JobScheduler/engine/objects/jobs//PATH/info
  // JobScheduler/engine/objects/jobs//PATH/description
  // JobScheduler/engine/objects/jobs//PATH/log
  // JobScheduler/engine/objects/jobs//PATH/log.snapshot
  // JobScheduler/engine/objects/jobs//PATH/tasks/TASKID/info
  // JobScheduler/engine/objects/jobs//PATH/tasks/TASKID/log
  // JobScheduler/engine/objects/job_chains//PATH/orders/ORDERID/log
  // JobScheduler/engine/objects/job_chains//PATH => XML-Konfiguration oder <show_job_chain>
  // JobScheduler/engine/objects/orders//PATH/ORDERID => XML-Konfiguration
  // JobScheduler/engine/objects/orders//PATH/ORDERID/info => <show_order>
  // JobScheduler/engine/objects/orders//PATH/ORDERID/log
  // JobScheduler/engine/objects/orders//PATH/ORDERID/log&historyId=.."

  // JobScheduler/engine/objects/folders/PATH => Unterordner
  // JobScheduler/engine/objects/jobs//PATH => Job-Konfiguration
  // JobScheduler/engine/objects/jobs//PATH/*description
  // JobScheduler/engine/objects/jobs//PATH?part=log
  // JobScheduler/engine/objects/jobs//PATH?part=log.snapshot
  // JobScheduler/engine/objects/tasks//PATH?id=TASKID&part=info
  // JobScheduler/engine/objects/tasks//PATH?id=TASKID&part=log
  // JobScheduler/engine/objects/job_chains//PATH => XML-Konfiguration
  // JobScheduler/engine/objects/job_chains//PATH?part=info => <show_job_chain>
  // JobScheduler/engine/objects/orders//PATH/ => Aufträge
  // JobScheduler/engine/objects/orders//PATH/ORDERID&part=log
  // JobScheduler/engine/objects/orders//PATH/ORDERID => XML-Konfiguration
  // JobScheduler/engine/objects/orders//PATH/ORDERID?part=info => <show_order>
  // JobScheduler/engine/objects/orders//PATH/ORDERID?part=log
  // JobScheduler/engine/objects/orders//PATH/ORDERID?historyId=..&part=log
  // JobScheduler/engine/objects/orders//PATH/ORDERID&part=log&historyId=.."

  // JobScheduler/engine/objects/folders/PATH => Unterordner
  // JobScheduler/engine/objects/job?job=PATH => Job-Konfiguration
  // JobScheduler/engine/objects/job.description?job=PATH
  // JobScheduler/engine/objects/job.log?job=PATH
  // JobScheduler/engine/objects/job.log.snapshot?job=PATH
  // JobScheduler/engine/objects/task.info?job=PATH&task=TASKID
  // JobScheduler/engine/objects/task.log?job?PATH&task=TASKID
  // JobScheduler/engine/objects/job_chain?job_chain=PATH => XML-Konfiguration
  // JobScheduler/engine/objects/job_chain.info?job_chain=PATH => <show_job_chain>
  // ...

  // ODER
  // JobScheduler/engine/folders//PATH/ liefert Inhalt des Pfads: Ordner, Jobs usw., nicht verschachtelt
  // JobScheduler/engine/folders//PATH/?deep=true wie vorher, aber verschachtelt
  // JobScheduler/engine/folders//PATH/*.job liefert Jobs
  // JobScheduler/engine/folders//PATH/*.job?deep=true wie vorher, aber verschachtelt
  // JobScheduler/engine/folders//PATH.job/log  Liefert fortlaufend bis Log beendet ist
  // JobScheduler/engine/folders//PATH.job/log.snapshot  liefert nur den aktuellen Stand
  // JobScheduler/engine/folders//PATH.job/description
  // JobScheduler/engine/folders//PATH.task/TASKID/log
  // JobScheduler/engine/folders//PATH.job_chain/orders/ORDERID/log
  // JobScheduler/engine/folders//PATH.job_chain/orders/ORDERID/log&history_id=..
  // KONFLIKT
  // JobScheduler/engine/folders//PATH.job_chain/orders/a.job/log

  // "/JobScheduler/engine/log" Hauptprotokoll
  // "/JobScheduler/engine/configuration.xml"
  // "/JobScheduler/engine/" liefert <show_state what="all,orders"/>

  // "/JobScheduler/z/" ?
  // "/JobScheduler/gui/"  Operations GUI
  // "/" verweist auf "/JobScheduler" -> "/JobScheduler/gui/"

  // ERLEDIGT:
  // "/JobScheduler/engine/command&command=XMLCOMMAND"
  // "/JobScheduler/engine/command"  POST

  // TODO Logs
  // TODO Fortlaufende Logs
  // TODO XML oder JSON
  // TODO Authentifizierung
  // TODO HTTPS
  // TODO WAR-Files
  // TODO GZIP
  // TODO Massentests: Viele Anfragen gleichzeitig. Anzahl der Threads soll klein bleiben.
}
