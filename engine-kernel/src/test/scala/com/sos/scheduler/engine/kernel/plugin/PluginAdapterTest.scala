package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.Guice._
import com.google.inject.Injector
import com.sos.scheduler.engine.common.xml.CppXmlUtils.loadXml
import com.sos.scheduler.engine.kernel.plugin.PluginAdapterTest._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class PluginAdapterTest extends FunSuite {
  private val pluginAdapter = new PluginAdapter(pluginConfiguration)
  private val errorPluginInstanceAdapter = new PluginAdapter(errorPluginConfiguration)
  private val injector: Injector = createInjector()

  test("initialize") {
    pluginAdapter.initialize(injector)
  }

  test("prepare") {
    pluginAdapter should not be 'prepared
    pluginAdapter.prepare()
    pluginAdapter shouldBe 'prepared
  }

  test("activate") {
    pluginAdapter should not be 'active
    pluginAdapter.activate()
    pluginAdapter shouldBe 'active
  }

  test("activate error") {
    errorPluginInstanceAdapter.initialize(injector)
    intercept[RuntimeException] { errorPluginInstanceAdapter.activate() }
  }

  test("tryActivate") {
    pluginAdapter shouldBe 'active
    pluginAdapter.tryActivate()
    pluginAdapter shouldBe 'active
    errorPluginInstanceAdapter.tryActivate()
    errorPluginInstanceAdapter should not be 'active
  }

  test("tryClose") {
    pluginAdapter shouldBe 'active
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
    pluginAdapter.toString should startWith ("Plugin ")
  }
}

private object PluginAdapterTest {
  private val pluginConfiguration = new PluginConfiguration(classOf[MockPlugin].getName, None)
  private val errorPluginConfiguration = new PluginConfiguration(classOf[ErrorMockPlugin].getName, None)
}
