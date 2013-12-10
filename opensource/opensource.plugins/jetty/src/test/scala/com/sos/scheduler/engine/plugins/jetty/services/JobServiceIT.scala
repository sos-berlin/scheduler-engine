package com.sos.scheduler.engine.plugins.jetty.services

import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests.javaResource
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JobServiceIT extends FunSuite with ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(testPackage = Some(classOf[JettyPlugin].getPackage))

  private lazy val jobResource = javaResource(injector).path("job").queryParam("job", "a")

  test("Read a job configuration") {
    val result = jobResource.path("configuration").accept(TEXT_XML_TYPE).get(classOf[String])
    result should include ("<job")
  }

  test("Read a job description") {
    val result = jobResource.path("description").accept(TEXT_PLAIN_TYPE).get(classOf[String])
    result should equal ("TEST-DESCRIPTION mit Ümläüten")
  }

  test("Read a job snapshot log") {
    val log = jobResource.path("log.snapshot").accept(TEXT_PLAIN_TYPE).get(classOf[String])
    log should include ("SCHEDULER-893")
  }
}
