package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import com.sos.scheduler.engine.plugins.jetty.JettyPluginConfiguration.prefixPath
import java.net.URI
import javax.ws.rs.core.MediaType._
import org.apache.log4j.Logger
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

import JettyPluginTest._

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class CommandResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  import CommandResourceTest._

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val commandResource = newAuthResource(contextUri+"/command")

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

object CommandResourceTest {
  private val logger = Logger.getLogger(classOf[CommandResourceTest])
  private val jettyPortNumber = 44440
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + prefixPath)
}
