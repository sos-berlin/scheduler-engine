package com.sos.scheduler.engine.plugins.webservice.services

import CommandServiceIT._
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester.normalizeUri
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sun.jersey.api.client.{ClientResponse, UniformInterfaceException}
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class CommandServiceIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  "Execute a command via POST" in {
    postCommand("<show_state><!--äöü--></show_state>") should include ("<state")
  }

  "Execute a show command via GET" in {
    getCommand("<show_state/>") should include ("<state")
  }

  "Execute a show command without XML syntax via GET" in {
    getCommand("show_state") should include ("<state")
    getCommand("s") should include ("<state")
  }

  "POST executes a modifying command" in {
    xml.XML.loadString(postCommand(modifyingCommand)).child(0).child(0) shouldEqual <ok/>
  }

  "GET inhibits a modifying command" in {
    def httpGetShouldForbid(command: String) {
      interceptHttpError(ClientResponse.Status.FORBIDDEN) {
        getCommand(command)
      }
    }
    httpGetShouldForbid(modifyingCommand)
    httpGetShouldForbid(strippedModifyingCommand)
  }

  "GET rejects a concatenated modifying command" in {
    interceptHttpError(ClientResponse.Status.BAD_REQUEST) {
      getCommand(s"<show_state/>$modifyingCommand")
    }
  }

  def postCommand(command: String) =
    webResource.path("/jobscheduler/engine/command").accept(TEXT_XML_TYPE).`type`(TEXT_XML_TYPE).post(classOf[String], command)

  def getCommand(command: String) =
    get[String](normalizeUri(s"/jobscheduler/engine/command?command=$command"), Accept = List(TEXT_XML_TYPE))
}

private object CommandServiceIT {
  private val strippedModifyingCommand = "check_folders"
  private val modifyingCommand = s"<$strippedModifyingCommand/>"

  private def interceptHttpError(status: ClientResponse.Status)(body: ⇒ Unit) {
    intercept[UniformInterfaceException](body).getResponse.getClientResponseStatus shouldEqual status
  }
}
