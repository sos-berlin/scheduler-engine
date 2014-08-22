package com.sos.scheduler.engine.plugins.jetty.tests.webxml

import com.google.common.base.Charsets.UTF_8
import com.google.common.io.Resources
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sun.jersey.api.client.ClientResponse
import com.sun.jersey.api.client.ClientResponse.Status.FORBIDDEN
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class WebXmlIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {
  private val baseDir = controller.environment.directory.getCanonicalFile
  private val configDir = controller.environment.configDirectory
  private val configDirName = configDir.getName

  override protected def checkedBeforeAll(): Unit = {
    controller.activateScheduler()
    prepareWebXml()
    val pluginSubsystem = injector.getInstance(classOf[PluginSubsystem])
    pluginSubsystem.activatePlugin(classOf[JettyPlugin])
    super.checkedBeforeAll()
  }

  private def prepareWebXml(): Unit = {
    val webXmlFile = configDir / "web.xml"
    webXmlFile.contentString = webXmlFile.contentString.replace("{{SCHEDULER_WORK_URL}}", baseDir.toURI.toURL.toString)
  }

  "Web server delivers integrated resource as without a web.xml" in {
    get[String]("/jobscheduler/z/index.html") shouldEqual Resources.toString(getClass.getResource("/com/sos/scheduler/engine/web/z/index.html"), UTF_8)
  }

  "Web server delivers external files described in web.xml" in {
    get[String](s"/jobscheduler/$configDirName/scheduler.xml") shouldEqual (configDir / "scheduler.xml").contentString
  }

  "Web server redirects directory url, even if access to target is forbidden" in {
    val path = s"/jobscheduler/$configDirName"
    withClue(s"Path $path") {
      val response = get[ClientResponse](path)
      assertResult(302)(response.getStatus)   // 302 Temporary Redirect
      val redirectedUrl = response.getLocation.toString
      assertResult(webResource.path(path) + "/")(redirectedUrl)
      assertResult(FORBIDDEN) {
        get[ClientResponse](redirectedUrl).getClientResponseStatus
      }
    }
  }

  "Web server do not list directory content" in {
    val path = s"/jobscheduler/$configDirName/"
    withClue(s"Path $path") {
      assertResult(FORBIDDEN) {
        get[ClientResponse](path).getClientResponseStatus
      }
    }
  }
}
