package com.sos.scheduler.engine.plugins.jetty.rest

import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests.{javaContextUri, newAuthResource}
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

/**JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class CommandResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  override val configurationPackage = classOf[JettyPlugin].getPackage
  private lazy val commandResource = newAuthResource(javaContextUri(injector) + "/command")

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
    result should include("<state");
  }
}
