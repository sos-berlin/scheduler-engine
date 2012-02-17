package com.sos.scheduler.engine.plugins.jetty.webxml

import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.google.common.base.Charsets.UTF_8
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.kernel.util.Time
import com.google.common.io.{Resources, Files}

@RunWith(classOf[JUnitRunner])
final class WebXmlTest extends ScalaSchedulerTest {
  private val baseDir = controller.environment.directory
  private val configDir = controller.environment.configDirectory
  private lazy val resource = newAuthResource(contextUri(injector))

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    prepareWebXml()
    val pluginSubsystem = injector.getInstance(classOf[PluginSubsystem])
    pluginSubsystem.activatePlugin(classOf[JettyPlugin])
    super.checkedBeforeAll(configMap)
  }

  private def prepareWebXml() {
    val webXmlFile = new File(configDir, "web.xml")
    val a = readFile(webXmlFile).replace("{{SCHEDULER_WORK_URL}}", baseDir.toURI.toURL.toString)
    Files.write(a, webXmlFile, UTF_8)
  }

  ignore("(for debugging only") {
    controller.waitForTermination(Time.of(3600))
  }

  test("Web server should deliver integrated resource as without a web.xml") {
    val expected = Resources.toString(getClass.getResource("/com/sos/scheduler/engine/web/z/index.html"), UTF_8)
    val a = resource.path("z/index.html").get(classOf[String])
    assert( a === expected)
  }

  test("Web server should deliver external files described in web.xml") {
    val resource = newAuthResource(contextUri(injector))
    val expected = readFile(new File(configDir, "scheduler.xml"))
    val a = resource.path(configDir.getName +"/scheduler.xml").get(classOf[String])
    assert( a === expected)
  }
  private def readFile(f: File) = Files.toString(f, UTF_8)
}
