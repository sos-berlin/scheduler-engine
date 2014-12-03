package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.plugins.jetty.JettyPluginIT._
import com.sos.scheduler.engine.plugins.jetty.test.HttpVerbRestrictionTester.{DefaultServletMethods, GetServletMethods}
import com.sos.scheduler.engine.plugins.jetty.test.{DirectoryListingTests, HttpVerbRestrictionTester, JettyPluginJerseyTester}
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.TestEnvironment
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JettyPluginIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester with DirectoryListingTests {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage),
    cppSettings = Map(CppSettingName.htmlDir → testDirectory.getPath))
  private lazy val verbTester = new HttpVerbRestrictionTester(webResource)

  "HTTP OPTIONS and TRACE" - {
    for ((path, verbs) <- PathsVerbs) path in {
      verbTester.checkPathForVerbs(path, verbs)
    }
  }

  "Directory access" - {
    addDirectoryListingTests(TestEnvironment.ConfigSubdirectoryName)
  }
}

private object JettyPluginIT {
  private val PathsVerbs = List[(String, Set[String])](
    "/PING" -> GetServletMethods,
    "/jobscheduler/" -> DefaultServletMethods,
    "/jobscheduler/unknown" -> DefaultServletMethods,
    "/jobscheduler/z/" -> DefaultServletMethods,
    "/jobscheduler/engine-cpp/" -> Set(),
    "/jobscheduler/engine/log" -> Set("OPTIONS", "GET", "HEAD"))
}
