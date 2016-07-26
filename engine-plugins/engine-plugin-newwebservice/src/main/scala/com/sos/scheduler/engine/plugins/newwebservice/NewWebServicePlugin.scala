package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Injector, Provides}
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
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
  injector: Injector)
extends Plugin {

  override def onPrepare() = {
    val myInjector = injector.createChildInjector(new AbstractModule {
      def configure() = {}

      @Provides @Singleton
      def newWebServicePluginConfiguration: NewWebServicePluginConfiguration =
        NewWebServiceConfigurationParser.parseString(toXml(pluginElement))
    })
    pluginSubsystem.pluginOptionByClass(classOf[JettyPlugin]) match {
      case Some(jettyPlugin) ⇒
        jettyPlugin.addExtension(
          JettyPluginExtension(
            modifyServletContextHandler = servletContextHandler ⇒ {
              servletContextHandler.addEventListener(new SprayServletContextInitializer(myInjector))
              servletContextHandler.addServlet(classOf[spray.servlet.Servlet30ConnectorServlet], "/*") sideEffect { _.setAsyncSupported(true) }
            }))
      case None ⇒
        val httpPort = schedulerConfiguration.httpPortOption getOrElse { throw new IllegalArgumentException("Missing -http-port=") }
        val httpAddress = new InetSocketAddress("0.0.0.0", httpPort)
        val webServer = new WebServer(httpAddress, myInjector)(myInjector.instance[ActorSystem], myInjector.instance[ExecutionContext])
        closer.registerAutoCloseable(webServer)
        webServer.start()
    }
  }
}
