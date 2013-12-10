package com.sos.scheduler.engine.kernel.plugin

import PluginAdapterTest._
import com.google.inject.Guice._
import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.kernel.scheduler.PrefixLogMock
import java.lang.RuntimeException
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers.startsWith
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class PluginAdapterTest extends FunSuite {
  private val log = PrefixLogMock.newLog(logger.delegate)
  private val pluginAdapter = new PluginAdapter(pluginConfiguration)
  private val errorPluginInstanceAdapter = new PluginAdapter(errorPluginConfiguration)
  private val injector: Injector = createInjector()

  test("initialize") {
    pluginAdapter.initialize(injector, log)
  }

  test("activate") {
    pluginAdapter should not be 'active
    pluginAdapter.activate()
    pluginAdapter should be ('active)
  }

  test("activate error") {
    errorPluginInstanceAdapter.initialize(injector, log)
    intercept[RuntimeException] { errorPluginInstanceAdapter.activate() }
  }

  test("tryActivate") {
    pluginAdapter should be ('active)
    pluginAdapter.tryActivate()
    pluginAdapter should be ('active)
    errorPluginInstanceAdapter.tryActivate()
    errorPluginInstanceAdapter should not be 'active
  }

  test("tryClose") {
    pluginAdapter should be ('active)
    pluginAdapter.tryClose()
    pluginAdapter should not be 'active
    errorPluginInstanceAdapter should not be 'active
    errorPluginInstanceAdapter.tryClose()
    errorPluginInstanceAdapter should not be 'active
  }

  test("xmlState") {
    loadXml(pluginAdapter.xmlState)
    loadXml(errorPluginInstanceAdapter.xmlState)
  }

  test("toString") {
    assertThat(pluginAdapter.toString, startsWith("Plugin "))
  }
}

private object PluginAdapterTest {
  val logger = Logger(getClass)
  val pluginConfiguration = new PluginConfiguration(classOf[MockPlugin].getName, ActivationMode.activateOnStart, None)
  val errorPluginConfiguration = new PluginConfiguration(classOf[ErrorMockPlugin].getName, ActivationMode.activateOnStart, None)
}

