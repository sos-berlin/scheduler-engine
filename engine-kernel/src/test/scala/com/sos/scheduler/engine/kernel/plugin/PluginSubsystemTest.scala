package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.{AbstractModule, Guice, Provides}
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.eventbus.EventBus
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.plugin.ActivationMode.activateOnStart
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystemTest._
import java.util.NoSuchElementException
import javax.xml.stream.XMLEventReader
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class PluginSubsystemTest extends FreeSpec {

  private lazy val injector = Guice.createInjector(new AbstractModule {
    def configure() = {}

    @Provides def pluginConfigurations: immutable.Seq[PluginConfiguration] = List(
      PluginConfiguration(classOf[APlugin].getName, activateOnStart, None),
      PluginConfiguration(classOf[BPlugin].getName, activateOnStart, None))

    @Provides def eventBus = mock[EventBus]
  })
  private lazy val pluginSubsystem = injector.apply[PluginSubsystem] sideEffect { _.initialize() }
  private lazy val aPlugin: APlugin = pluginSubsystem.pluginByClass(classOf[APlugin])
  private lazy val bPlugin: BPlugin = pluginSubsystem.pluginByClass(classOf[BPlugin])

  "pluginByClass" in {
    aPlugin
    bPlugin
    intercept[NoSuchElementException] { pluginSubsystem.pluginByClass(classOf[UnregisteredPlugin]) }
  }

  "pluginAdapterByClassName" in {
    pluginSubsystem.pluginAdapterByClassName(classOf[APlugin].getName).pluginInstance shouldEqual aPlugin
    intercept[NoSuchElementException] { pluginSubsystem.pluginAdapterByClassName("UNKNOWN") }
  }

  "xmlNamespaceToPlugins" in {
    pluginSubsystem.xmlNamespaceToPlugins(Namespace) shouldEqual Some(bPlugin)
    pluginSubsystem.xmlNamespaceToPlugins("UNKNOWN") shouldEqual None
  }

  "plugins" in {
    pluginSubsystem.plugins[Plugin].toSet shouldEqual Set(aPlugin, bPlugin)
    pluginSubsystem.plugins[NamespaceXmlPlugin] shouldEqual List(bPlugin)
  }
}

private object PluginSubsystemTest {
  private val Namespace = "http://TEST-NAMESPACE"

  private class APlugin extends Plugin

  private class BPlugin extends NamespaceXmlPlugin {
    def xmlNamespace = Namespace
    def parseOnReturnCodeXml(node: JobNode, xmlEventReader: XMLEventReader) = throw new NotImplementedError
  }

  private class UnregisteredPlugin extends Plugin
}
