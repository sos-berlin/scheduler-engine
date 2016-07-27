package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Injector, Provides}
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersCloser
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.xml.XmlUtils.toXml
import com.sos.scheduler.engine.kernel.plugin.{Plugin, PluginSubsystem, Plugins, UseGuiceModule}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.{JettyPlugin, JettyPluginExtension}
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

  override def onPrepare() =
    pluginSubsystem.pluginOptionByClass(classOf[JettyPlugin]) match {
      case Some(jettyPlugin) ⇒ runWithJetty(jettyPlugin)
      case None ⇒ runWithSprayWebServer()
    }

  private def runWithJetty(jettyPlugin: JettyPlugin): Unit =
    jettyPlugin.addExtension(
      JettyPluginExtension(
        modifyServletContextHandler = servletContextHandler ⇒ {
          servletContextHandler.addEventListener(new SprayServletContextInitializer(myInjector))
          servletContextHandler.addServlet(classOf[spray.servlet.Servlet30ConnectorServlet], "/*") sideEffect { _.setAsyncSupported(true) }
        }))

  private def runWithSprayWebServer(): Unit = {
    val httpPort = schedulerConfiguration.httpPortOption getOrElse { throw new IllegalArgumentException("Missing -http-port=") }
    val httpAddress = new InetSocketAddress("0.0.0.0", httpPort)
    val webServer = new EngineWebServer(httpAddress, myInjector)
    closer.registerAutoCloseable(webServer)
    webServer.start()
  }
}
