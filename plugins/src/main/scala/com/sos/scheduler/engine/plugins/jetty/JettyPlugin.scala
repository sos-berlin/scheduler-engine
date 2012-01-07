package com.sos.scheduler.engine.plugins.jetty

import javax.inject.Inject
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin
import com.sos.scheduler.engine.kernel.util.XmlUtils
import com.sun.jersey.guice.JerseyServletModule
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer
import com.google.inject.Guice
import com.google.inject.servlet.{GuiceFilter, GuiceServletContextListener}
import com.sos.scheduler.engine.kernel.scheduler.HasGuiceModule
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
      bind(classOf[CommandResource])
      bind(classOf[ObjectsResource])
      //serve("/objects/*.job/log").`with`(classOf[LogServlet])
      serve("/*").`with`(classOf[GuiceContainer]) // Route all requests through GuiceContainer
    }
  }


  // TODO URIs und REST
  // Alle REST-Aufrufe liefern XML oder JSON.

  // KONTEXTE
  // /JobScheduler/engine/
  // /JobScheduler/gui/

  // "/JobScheduler/engine/objects/jobs/PATH" liefert Jobs
  // "/JobScheduler/engine/objects/folders/PATH" liefert Ordner
  // ...
  // "/JobScheduler/engine/objects/jobs//PATH/log&snapshot=true"  snapshot liefert nur den aktuellen Stand, sonst fortlaufend bis Log beendet ist.
  // "/JobScheduler/engine/objects/jobs//PATH/description"
  // "/JobScheduler/engine/objects/jobs//PATH/task/TASKID/log"
  // "/JobScheduler/engine/objects/job_chains//PATH/orders/ORDERID/log"
  // "/JobScheduler/engine/objects/job_chains//PATH/orders/ORDERID/log&historyId=.."
  // ODER
  // "/JobScheduler/engine/objects//PATH/" liefert Inhalt des Pfads: Ordner, Jobs usw., nicht verschachtelt.
  // "/JobScheduler/engine/objects//PATH/?deep=true" wie vorher, aber verschachtelt
  // "/JobScheduler/engine/objects//PATH/*.job" liefert Jobs
  // "/JobScheduler/engine/objects//PATH/*.job?deep=true" wie vorher, aber verschachtelt
  // "/JobScheduler/engine/objects//PATH.job/log&snapshot=true"  snapshot liefert nur den aktuellen Stand, sonst fortlaufend bis Log beendet ist.
  // "/JobScheduler/engine/objects//PATH.job/description"
  // "/JobScheduler/engine/objects//PATH.task/TASKID/log"
  // "/JobScheduler/engine/objects//PATH.job_chain/orders/ORDERID/log"
  // "/JobScheduler/engine/objects//PATH.job_chain/orders/ORDERID/log&history_id=.."

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
