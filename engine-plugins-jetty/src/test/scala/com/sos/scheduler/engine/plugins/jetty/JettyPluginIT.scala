package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.plugins.jetty.JettyPluginIT._
import com.sos.scheduler.engine.plugins.jetty.test.HttpVerbRestrictionTester.{DefaultServletMethods, GetServletMethods}
import com.sos.scheduler.engine.plugins.jetty.test.{DirectoryListingTests, HttpVerbRestrictionTester, JettyPluginJerseyTester}
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.TestEnvironment
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sun.jersey.api.client.ClientResponse
import com.sun.jersey.api.client.ClientResponse.Status.{NOT_FOUND, TEMPORARY_REDIRECT}
import java.net.URI
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JettyPluginIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester with DirectoryListingTests {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage),
    cppSettings = Map(CppSettingName.WebDirectory → testDirectory.getPath))
  private lazy val verbTester = new HttpVerbRestrictionTester(webResource)

  "HTTP OPTIONS and TRACE" - {
    for ((path, verbs) <- PathsVerbs) path in {
      verbTester.checkPathForVerbs(path, verbs)
    }
  }

  "Directory access" - {
    addDirectoryListingTests(TestEnvironment.ConfigSubdirectoryName)
  }

  "/ is forwarded to /jobscheduler/operations_gui" in {
    get[ClientResponse]("/") should have (
      'status (TEMPORARY_REDIRECT.getStatusCode),
      'location (new URI("/jobscheduler/operations_gui/"))
    )
  }

//  "/jobscheduler is forwarded to /jobscheduler/operations_gui/" in {
//    get[ClientResponse]("/jobscheduler") should have (
//      'status (TEMPORARY_REDIRECT.getStatusCode),
//      'location (new URI("/jobscheduler/operations_gui/"))
//    )
//  }
//
//  "/jobscheduler/ is forwarded to /jobscheduler/operations_gui/" in {
//    get[ClientResponse]("/jobscheduler/") should have (
//      'status (TEMPORARY_REDIRECT.getStatusCode),
//      'location (new URI("/jobscheduler/operations_gui/"))
//    )
//  }

  "Nothing else is forwarded" in {
    get[ClientResponse]("/dontexists").getStatus shouldEqual NOT_FOUND.getStatusCode
  }
}

private object JettyPluginIT {
  private val PathsVerbs = List[(String, Set[String])](
    "/" -> GetServletMethods,
    "/PING" -> GetServletMethods,
    "/jobscheduler/" -> DefaultServletMethods,
    "/jobscheduler/unknown" -> DefaultServletMethods,
    "/jobscheduler/z/" -> DefaultServletMethods,
    "/jobscheduler/engine-cpp/" -> Set(),
    "/jobscheduler/engine/log" -> Set("OPTIONS", "GET", "HEAD"))
}
