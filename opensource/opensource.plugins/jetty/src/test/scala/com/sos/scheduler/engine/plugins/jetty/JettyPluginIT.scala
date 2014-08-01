package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.plugins.jetty.JettyPluginIT._
import com.sos.scheduler.engine.plugins.jetty.test.HttpVerbRestrictionTester.{DefaultServletMethods, GetServletMethods}
import com.sos.scheduler.engine.plugins.jetty.test.{HttpVerbRestrictionTester, JettyPluginJerseyTester}
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JettyPluginIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(testClass = getClass, testPackage = Some(Tests.testPackage))
  private lazy val verbTester = new HttpVerbRestrictionTester(webResource)

  for ((path, verbs) <- PathsVerbs) s"HTTP OPTIONS/TRACE $path" in {
    verbTester.checkPathForVerbs(path, verbs)
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
