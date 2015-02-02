package com.sos.scheduler.engine.plugins.newwebservice

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.xml.XmlUtils.toXml
import com.sos.scheduler.engine.kernel.plugin.{Plugin, PluginSubsystem, Plugins}
import com.sos.scheduler.engine.plugins.jetty.{JettyPlugin, JettyPluginExtension}
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import javax.inject.{Inject, Named}
import org.w3c.dom.Element

/**
 * @author Joacim Zschimmer
 */
final class NewWebServicePlugin @Inject private(
  pluginSubsystem: PluginSubsystem,
  @Named(Plugins.configurationXMLName) pluginElement: Element,
  injector: Injector)
extends Plugin {

  override def onPrepare() = {
    val myInjector = injector.createChildInjector(new ScalaAbstractModule {
      def configure() = {
        bindClass[NewWebServicePluginConfiguration] toInstance NewWebServiceConfigurationParser.parseString(toXml(pluginElement))
      }
    })
    pluginSubsystem.pluginByClass(classOf[JettyPlugin]).addExtension(
      JettyPluginExtension(
        modifyServletContextHandler = servletContextHandler ⇒ {
          servletContextHandler.addEventListener(new SprayServletContextInitializer(myInjector))
          servletContextHandler.addServlet(classOf[spray.servlet.Servlet30ConnectorServlet], "/*") sideEffect { _.setAsyncSupported(true) }
        }))
  }
}
