package com.sos.scheduler.engine.web

import com.google.common.io.Resources.getResource
import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.web.WebIT._
import java.net.URI
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.selenium.Firefox

/**
 * @author Joacim Zschimmer
 */
//TODO Nicht automatisch unter Maven ausführen: @RunWith(classOf[JUnitRunner])
final class WebIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester with Firefox {

  onClose {
    webDriver.quit()
  }

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    cppSettings = Map(CppSettingName.WebDirectoryUrl → WebResource))

  "index.html" in {
    get[String](s"/jobscheduler/z/index.html") should include ("<html")
  }

  "GUI" - {
    "Load page and check title" in {
      go to uri("/jobscheduler/z/index.html")
      pageTitle shouldEqual s"Scheduler 127.0.0.1:$port"
      webDriver.switchTo.frame("left_frame")
    }

    "Jobchain" in {
      pendingUntilFixed {
        find(XPathQuery("//tr[@class='job_chain']")).get.text shouldEqual "/test-jobchain"
      }
    }

    "Job in jobchain" in {
      val span3 = find(XPathQuery("//tr[@class='job']/td[3]/span")).get
      span3.text shouldEqual "/test-job"
      span3.attribute("class") shouldEqual Some("job_missing")
      find(XPathQuery("//tr[@class='job']/td[4]")).get.text shouldEqual "missing"
    }
  }

  private def uri(u: String) = webResource.uri(new URI(u)).getURI.toString
}

private object WebIT {
  private val WebResource = getResource("com/sos/scheduler/engine/web/z").toExternalForm stripSuffix "/z"  // IntelliJ has different directories out/test and out/production
}
