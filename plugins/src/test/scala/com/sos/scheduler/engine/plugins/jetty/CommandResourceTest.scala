package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

import JettyPluginTests._

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class CommandResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  private lazy val commandResource = newAuthResource(javaContextUri(injector)+"/command")

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler("-e")
    super.checkedBeforeAll(configMap)
  }

  test("Execute a command via POST ") {
    val result = commandResource.accept(TEXT_XML_TYPE).`type`(TEXT_XML_TYPE).post(classOf[String], "<show_state/>");
    checkCommandResult(result)
  }

  test("Execute a command via GET ") {
    val result = commandResource.queryParam("command", "<show_state/>").accept(TEXT_XML_TYPE).get(classOf[String])
    checkCommandResult(result)
  }

  private def checkCommandResult(result: String) {
    result should include ("<state");
  }
}
