package com.sos.scheduler.engine.plugins.jetty

import JettyPlugin._
import com.sos.scheduler.engine.kernel.plugin.{Plugin, UseGuiceModule, AbstractPlugin}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.SchedulerConfigurationAdapter
import com.sos.scheduler.engine.plugins.jetty.configuration.injection.JettyModule
import javax.inject.{Named, Inject}
import org.eclipse.jetty.servlet.ServletContextHandler
import org.w3c.dom.Element

/** JS-795: Einbau von Jetty in den JobScheduler. */
@UseGuiceModule(classOf[JettyModule])
final class JettyPlugin @Inject private(
    @Named(Plugin.configurationXMLName) pluginElement: Element,
    schedulerConfiguration: SchedulerConfiguration)
extends AbstractPlugin {

  private val webServer = new WebServer(myJettyConfiguration)

  private def myJettyConfiguration = {
    val c = SchedulerConfigurationAdapter.jettyConfiguration(pluginElement, schedulerConfiguration)
    c.copy(handlers = newRootContextHandler() +: c.handlers)
  }

  override def activate() {
    webServer.start()
  }

  override def close() {
    webServer.close()
  }
}

object JettyPlugin {
  private def newRootContextHandler() = {
    val result = new ServletContextHandler(ServletContextHandler.SESSIONS)
    result.setContextPath("/")
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
