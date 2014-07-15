package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.kernel.plugin._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin._
import com.sos.scheduler.engine.plugins.jetty.configuration.SchedulerConfigurationAdapter
import com.sos.scheduler.engine.plugins.jetty.configuration.injection.JettyModule
import java.net.BindException
import javax.inject.{Inject, Named}
import org.eclipse.jetty.servlet.ServletContextHandler
import org.w3c.dom.Element

/** JS-795: Einbau von Jetty in den JobScheduler. */
@UseGuiceModule(classOf[JettyModule])
final class JettyPlugin @Inject private(
    @Named(Plugins.configurationXMLName) pluginElement: Element,
    schedulerConfiguration: SchedulerConfiguration)
extends Plugin
with ExtensionRegister[JettyPluginExtension]
with HasCloser {

  private var webServer: WebServer = null

  override def onActivate() {
    webServer = new WebServer(myJettyConfiguration).registerCloseable
    val portNumbersString = webServer.portNumbers mkString " "
    if (portNumbersString.nonEmpty) logger.info(s"HTTP port $portNumbersString")
    else logger.warn(s"No HTTP port seems to be configured")
    try {
      webServer.start()
    }
    catch {
      case e: BindException if portNumbersString.nonEmpty ⇒
        throw new RuntimeException(s"$e (TCP port $portNumbersString?})", e)
    }
  }

  private def myJettyConfiguration = {
    val conf = SchedulerConfigurationAdapter.jettyConfiguration(pluginElement, schedulerConfiguration)
    conf.copy(
      handlers = newRootContextHandler() +: conf.handlers,
      servletContextHandlerModifiers = extensions map { _.modifyServletContextHandler })
  }

  def portNumber: Int = {
    if (webServer == null) throw new IllegalStateException("JettyPlugin has not been activated")
    webServer.portNumbers.headOption getOrElse sys.error("JettyPlugin has no TCP port configured")
  }
}

object JettyPlugin {
  private val logger = Logger(getClass)

  private def newRootContextHandler() = {
    val result = new ServletContextHandler(ServletContextHandler.SESSIONS)
    result.setContextPath("/")
    result.addFilter(classOf[VerbRestrictionFilter], "/*", null)
    result.addServlet(classOf[RootForwardingServlet], "/")
    result
  }
}

// jobscheduler/engine/  Übersicht über den Zustand und weitere URIs
// jobscheduler/engine/log  Hauptprotokoll
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
