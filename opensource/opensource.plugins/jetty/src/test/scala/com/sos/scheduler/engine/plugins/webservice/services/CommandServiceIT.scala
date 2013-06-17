package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.plugins.jetty.tests.commons.JettyPluginTests.javaResource
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class CommandServiceIT extends ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(testPackage = Some(Tests.testPackage))

  private lazy val commandResource = javaResource(injector).path("command")

  test("Execute a command via POST") {
    pending
    val result = commandResource.accept(TEXT_XML_TYPE).`type`(TEXT_XML_TYPE).post(classOf[String], "<show_state><!--äöü--></show_state>")
    checkCommandResult(result)
  }

  test("Execute a command via GET") {
    val result = commandResource.queryParam("command", "<show_state/>").accept(TEXT_XML_TYPE).get(classOf[String])
    checkCommandResult(result)
  }

  private def checkCommandResult(result: String) {
    result should include("<state")
  }
}
