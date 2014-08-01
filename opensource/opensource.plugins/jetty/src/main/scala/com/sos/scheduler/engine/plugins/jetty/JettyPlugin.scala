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
    conf.copy(rootServletContextHandlerModifiers = extensions map { _.modifyServletContextHandler })
  }

  def portNumber: Int = {
    if (webServer == null) throw new IllegalStateException("JettyPlugin has not been activated")
    webServer.portNumbers.headOption getOrElse sys.error("JettyPlugin has no TCP port configured")
  }
}

object JettyPlugin {
  private val logger = Logger(getClass)
}
