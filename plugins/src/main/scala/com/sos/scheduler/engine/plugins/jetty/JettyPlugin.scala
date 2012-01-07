package com.sos.scheduler.engine.plugins.jetty

import javax.inject.Inject
import com.google.inject.Guice
import com.google.inject.servlet.{GuiceFilter, GuiceServletContextListener}
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin
import com.sos.scheduler.engine.kernel.scheduler.HasGuiceModule
import com.sos.scheduler.engine.kernel.util.XmlUtils
import com.sos.scheduler.engine.plugins.jetty.bodywriters.XmlElemWriter
import com.sun.jersey.guice.JerseyServletModule
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer
import org.apache.log4j.Logger
import org.eclipse.jetty.server.Server
import org.eclipse.jetty.servlet.{DefaultServlet, ServletContextHandler}
import org.w3c.dom.Element

/**JS-795: Einbau von Jetty in den JobScheduler. */
final class JettyPlugin @Inject()(pluginElement: Element, hasGuiceModule: HasGuiceModule) extends AbstractPlugin {
  import JettyPlugin._

  private lazy val server = {
    val port = XmlUtils.intXmlAttribute(pluginElement, "port")
    val contextHandler = {
      val o = new ServletContextHandler(ServletContextHandler.SESSIONS)
      o.setContextPath(contextPath)
      o.addEventListener(new GuiceServletContextListener {
        val schedulerModule = hasGuiceModule.getGuiceModule
        def getInjector = Guice.createInjector(schedulerModule, newServletModule())
        //Funktioniert nicht: def getInjector = injector.createChildInjector(newServletModule())
      })
      o.addFilter(classOf[GuiceFilter], "/*", null)  // Reroute all requests through this filter
      o.addServlet(classOf[DefaultServlet], "/")   // Failing to do this will cause 404 errors. This is not needed if web.xml is used instead.
      o
    }
    val server = new Server(port)
    server.setHandler(contextHandler)
    server
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
  private val logger: Logger = Logger.getLogger(classOf[JettyPlugin])
  val contextPath = "/JobScheduler/engine"

  private def newServletModule() = new JerseyServletModule {
    override def configureServlets() {
      serve("/cpp/*").`with`(classOf[CppServlet])
      serveRegex("/objects/"+JobLogServlet.PathInfoRegex).`with`(classOf[JobLogServlet])
      serveRegex("/objects/"+OrderLogServlet.PathInfoRegex).`with`(classOf[OrderLogServlet])
      serveRegex("/log").`with`(classOf[MainLogServlet])
      serve("/*").`with`(classOf[GuiceContainer]) // Route all requests through GuiceContainer
      bind(classOf[CommandResource])
      bind(classOf[JobResource])
      bind(classOf[JobsResource])
      bind(classOf[XmlElemWriter])
    }
  }


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
  // TODO Request-Log
  // TODO WAR-Files
  // TODO GZIP
  // TODO Massentests: Viele Anfragen gleichzeitig. Anzahl der Threads soll klein bleiben.
}
