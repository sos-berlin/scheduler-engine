package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.{AbstractModule, Guice, Provides}
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.eventbus.EventBus
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystemTest._
import java.util.NoSuchElementException
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
    @Provides def pluginConfigurations: immutable.Seq[PluginConfiguration] = List(
      PluginConfiguration(classOf[APlugin].getName, None),
      PluginConfiguration(classOf[BPlugin].getName, None))

    @Provides def eventBus = mock[EventBus]
  })
  private lazy val pluginSubsystem = injector.instance[PluginSubsystem] sideEffect { _.initialize() }
  private lazy val aPlugin: APlugin = pluginSubsystem.plugin[APlugin]
  private lazy val bPlugin: BPlugin = pluginSubsystem.plugin[BPlugin]

  "pluginByClass" in {
    aPlugin
    bPlugin
    intercept[NoSuchElementException] { pluginSubsystem.plugin[UnregisteredPlugin] }
  }

  "pluginAdapterByClassName" in {
    pluginSubsystem.classNameToPluginAdapter(classOf[APlugin].getName).pluginInstance shouldEqual aPlugin
    intercept[NoSuchElementException] { pluginSubsystem.classNameToPluginAdapter("UNKNOWN") }
  }

  "xmlNamespaceToPlugins" in {
    pluginSubsystem.xmlNamespaceToPlugins[NamespaceXmlPlugin](Namespace) shouldEqual Some(bPlugin)
    pluginSubsystem.xmlNamespaceToPlugins[Plugin](Namespace) shouldEqual Some(bPlugin)
    pluginSubsystem.xmlNamespaceToPlugins[BPlugin](Namespace) shouldEqual Some(bPlugin)
    pluginSubsystem.xmlNamespaceToPlugins[APlugin](Namespace) shouldEqual None
    pluginSubsystem.xmlNamespaceToPlugins[NamespaceXmlPlugin]("UNKNOWN") shouldEqual None
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
  }

  private class UnregisteredPlugin extends Plugin
}
