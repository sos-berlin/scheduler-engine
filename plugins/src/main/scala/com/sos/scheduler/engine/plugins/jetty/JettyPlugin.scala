package com.sos.scheduler.engine.plugins.jetty

import javax.inject.Inject
import com.google.inject.Injector
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin
import com.sos.scheduler.engine.kernel.util.XmlUtils
import org.apache.log4j.Logger
import org.w3c.dom.Element
import com.sun.jersey.guice.JerseyServletModule
import com.google.inject.AbstractModule._
import com.google.inject.servlet.ServletModule._
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer

/**JS-795: Einbau von Jetty in den JobScheduler. */
final class JettyPlugin @Inject()(pluginElement: Element, injector: Injector) extends AbstractPlugin {
  import JettyPlugin._
  private val port = XmlUtils.intXmlAttribute(pluginElement, "port")
  private lazy val server = new MyJettyServer(port, contextPath, injector, servletModule)

  override def activate() {
    server.start()
  }

  override def close() {
    server.close()
  }
}

object JettyPlugin {
  private val logger: Logger = Logger.getLogger(classOf[JettyPlugin])
  private val contextPath = "/JobScheduler"
  private val servletModule = new JerseyServletModule {
    override protected def configureServlets() {
      //serve("/command.servlet").`with`(classOf[CommandServlet])
      //bind(classOf[EmptyResource]) // Must configure at least one JAX-RS resource or the server will fail to start.
      bind(classOf[CommandResource])
      bind(classOf[ObjectsResource])
      serve("/*").`with`(classOf[GuiceContainer]) // Route all requests through GuiceContainer
    }
  }


  // TODO URIs und REST
  // Alle REST-Aufrufe liefern XML oder JSON.
  // "/jobs/PATH" liefert Jobs
  // "/folders/PATH" liefert Ordner
  // ...
  // "/jobs//PATH/log&snapshot=true"  snapshot liefert nur den aktuellen Stand, sonst fortlaufend bis Log beendet ist.
  // "/jobs//PATH/description"
  // "/jobs//PATH/task/TASKID/log"
  // "/job_chains//PATH/orders/ORDERID/log"
  // "/job_chains//PATH/orders/ORDERID/log&historyId=.."
  // ODER
  // "/objects/" liefert Objekte: Ordner, Jobs usw., nicht verschachtelt.
  // "/objects//PATH/" liefert Inhalt des Pfads
  // "/objects//JOBPATH.job/log&snapshot=true"  snapshot liefert nur den aktuellen Stand, sonst fortlaufend bis Log beendet ist.
  // "/objects//JOBPATH.job/description"
  // "/objects//JOBPATH.task/TASKID/log"
  // "/objects//JOBCHAINPATH.jobChain/orders/ORDERID/log"
  // "/objects//JOBCHAINPATH.jobChain/orders/ORDERID/log&historyId=.."
  //
  // "/log" Hauptprotokoll
  // "/configuration.xml"
  // "/static/" ?
  // "/z/" ?
  // "/" liefert <show_state what="all,orders"/>

  // ERLEDIGT:
  // "/command&command=XMLCOMMAND"
  // "/command"  POST

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
