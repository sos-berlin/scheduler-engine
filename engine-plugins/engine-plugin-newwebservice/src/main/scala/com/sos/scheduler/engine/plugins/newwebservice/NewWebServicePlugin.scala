package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Injector, Provides}
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersCloser
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.xml.XmlUtils.toXml
import com.sos.scheduler.engine.kernel.plugin.{Plugin, PluginSubsystem, Plugins, UseGuiceModule}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import java.net.InetSocketAddress
import javax.inject.{Inject, Named, Singleton}
import org.w3c.dom.Element
import scala.concurrent.ExecutionContext

/**
 * @author Joacim Zschimmer
 */
@UseGuiceModule(classOf[NewWebServiceModule])
final class NewWebServicePlugin @Inject private(
  pluginSubsystem: PluginSubsystem,
  @Named(Plugins.configurationXMLName) pluginElement: Element,
  schedulerConfiguration: SchedulerConfiguration,
  schedulerInjector: Injector)
  (implicit
    actorSystem: ActorSystem,
    executionContext: ExecutionContext)
extends Plugin {

  private val conf = NewWebServiceConfigurationParser.parseString(toXml(pluginElement))
  private val myInjector = schedulerInjector.createChildInjector(new AbstractModule {
    def configure() = {}

    @Provides @Singleton
    def newWebServicePluginConfiguration: NewWebServicePluginConfiguration = conf
  })

  override def onPrepare() = runWithSprayWebServer()

  private def runWithSprayWebServer(): Unit = {
    for (httpPort ← schedulerConfiguration.httpPortOption) {
      val httpAddress = new InetSocketAddress("0.0.0.0", httpPort)
      val webServer = new EngineWebServer(httpAddress, myInjector)
      closer.registerAutoCloseable(webServer)
      webServer.start() await 60.s
    }
  }
}
