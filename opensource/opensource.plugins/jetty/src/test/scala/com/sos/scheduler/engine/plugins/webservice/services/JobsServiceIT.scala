package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.plugins.jetty.tests.commons.JettyPluginTests.javaResource
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sun.jersey.api.client.GenericType
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JobsServiceIT extends FunSuite with ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  private lazy val jobsResource = javaResource(injector).path("jobs")

  test("Read job list") {
    jobsResource.accept(APPLICATION_JSON_TYPE).get(new GenericType[Set[String]]() {})  should equal (Set("/a"))
  }
}
