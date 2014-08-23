package com.sos.scheduler.engine.plugins.jetty.tests.webxml

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.test.{DirectoryListingTests, JettyPluginJerseyTester}
import com.sos.scheduler.engine.test.TestEnvironment
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class WebXmlIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester with DirectoryListingTests {

  private lazy val baseDir = controller.environment.directory.getCanonicalFile
  private lazy val configDir = controller.environment.configDirectory
  private val webDirectoryName = TestEnvironment.ConfigSubdirectoryName

  override protected def checkedBeforeAll(): Unit = {
    controller.activateScheduler()
    prepareWebXml()
    val pluginSubsystem = injector.getInstance(classOf[PluginSubsystem])
    pluginSubsystem.activatePlugin(classOf[JettyPlugin])
    super.checkedBeforeAll()
  }

  private def prepareWebXml(): Unit = {
    assert(webDirectoryName == configDir.getName)
    val webXmlFile = configDir / "web.xml"
    webXmlFile.contentString = webXmlFile.contentString.replace("{{SCHEDULER_WORK_URL}}", baseDir.toURI.toURL.toString)
  }

  "Web server delivers external files described in web.xml" in {
    get[String](s"/jobscheduler/$webDirectoryName/scheduler.xml") shouldEqual (configDir / "scheduler.xml").contentString
  }

  "Directory access" - {
    addDirectoryListingTests(webDirectoryName)
  }
}
