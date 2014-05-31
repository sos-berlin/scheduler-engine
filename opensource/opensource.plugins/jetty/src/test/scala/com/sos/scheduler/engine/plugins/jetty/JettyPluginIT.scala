package com.sos.scheduler.engine.plugins.jetty

import JettyPluginIT._
import com.google.common.base.Splitter
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sun.jersey.api.client.ClientResponse
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._

@RunWith(classOf[JUnitRunner])
final class JettyPluginIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(testClass = getClass, testPackage = Some(Tests.testPackage))

  for ((path, verbs) <- PathsVerbs) s"HTTP OPTIONS/TRACE $path" in {
    checkPath(path, verbs)
  }

  private def checkPath(path: String, verbSet: Set[String]) {
    httpTraceShouldBeNotAllowed(path)
    httpOptionShouldReturnVerbs(path, verbSet)
  }

  private def httpTraceShouldBeNotAllowed(path: String) {
    webResource.path(path).method("TRACE", classOf[ClientResponse]).getClientResponseStatus should
      (equal (ClientResponse.Status.METHOD_NOT_ALLOWED) or equal (ClientResponse.Status.FORBIDDEN))
  }

  private def httpOptionShouldReturnVerbs(path: String, verbSet: Set[String]) {
    val response = webResource.path(path).options(classOf[ClientResponse])
    if (verbSet.isEmpty) {
      response.getClientResponseStatus shouldEqual ClientResponse.Status.METHOD_NOT_ALLOWED
    } else {
      response.getClientResponseStatus shouldEqual ClientResponse.Status.OK
      val allowStrings = webResource.path(path).options(classOf[ClientResponse]).getHeaders.get("Allow")
      allowStrings.size shouldEqual 1
      (AllowSplitter split allowStrings(0)).toSet shouldEqual verbSet
    }
  }
}

private object JettyPluginIT {
  /** Methods as of [[org.eclipse.jetty.servlet.DefaultServlet]]doOptions() implementing POST as GET. */
  private val DefaultServletMethods = Set("OPTIONS", "GET", "HEAD", "POST")
  private val PathsVerbs = List[(String, Set[String])](
    "/" -> Set("OPTIONS", "GET", "HEAD"),
    "/x" -> Set("OPTIONS", "GET", "HEAD"),
    "/jobscheduler" -> DefaultServletMethods,
    "/jobscheduler/" -> DefaultServletMethods,
    "/jobscheduler/unknown" -> DefaultServletMethods,
    "/jobscheduler/z/" -> DefaultServletMethods,
    "/jobscheduler/engine-cpp/" -> Set(),
    "/jobscheduler/engine/log" -> Set("OPTIONS", "GET", "HEAD"))
  private val AllowSplitter = (Splitter on "[\r\n \t]*,[\r\n \t]*".r.pattern).trimResults.omitEmptyStrings
}
