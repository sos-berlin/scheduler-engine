package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester.normalizeUri
import com.sos.scheduler.engine.plugins.jetty.test.{JettyPluginJerseyTester, ProvideUmlautJob}
import com.sos.scheduler.engine.plugins.webservice.services.CommandServiceIT._
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sun.jersey.api.client.{ClientResponse, UniformInterfaceException}
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class CommandServiceIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester with ProvideUmlautJob {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  "Execute a command via POST" in {
    postCommand("<show_state><!--äöü--></show_state>") should include ("<state")
    controller.toleratingErrorCodes(_ ⇒ true) {
      postCommand("<modify_order job_chain='/A' order='äöüß' suspended='no'/>") should include ("SCHEDULER-162  There is no order äöüß in job chain")
      postCommand("<invalid-äöü/>") should include ("invalid-äöü")  // This succeeds with @Produces(Array("text/xml")), too ?
    }
  }

  "Execute a show command via GET" in {
    getCommand("<show_state/>") should include ("<state")
    getCommand("<s/>") should include ("<state")
    controller.toleratingErrorCodes(_ ⇒ true) {
      getCommand("<show_äöü/>") should include ("show_äöü")  // This succeeds with @Produces(Array("text/xml")), too ?
    }
  }

  "Execute a show command without XML syntax via GET" in {
    getCommand("show_state") should include ("<state")
    getCommand("s") should include ("<state")
  }

  "POST executes a modifying command" in {
    SafeXML.loadString(postCommand(ModifyingCommand)).child(0).child(0) shouldEqual <ok/>
  }

  "GET inhibits a modifying command" in {
    def httpGetShouldForbid(command: String): Unit = {
      interceptHttpError(ClientResponse.Status.FORBIDDEN) {
        getCommand(command)
      }
    }
    httpGetShouldForbid(ModifyingCommand)
    httpGetShouldForbid(StrippedModifyingCommand)
    httpGetShouldForbid("<start_job job='/a'/>")
  }

  "GET rejects a concatenated modifying command" in {
    interceptHttpError(ClientResponse.Status.BAD_REQUEST) {
      getCommand(s"<show_state/>$ModifyingCommand")
    }
  }

//  "Umlauts in job name, Windows only" in {
//    if (isUnix) pending  // Unix encodes filenames with UTF-8 but JobScheduler decodes then with ISO-8859-1 (see JS-1374)
//    pending // Jenkins fails due to invalid encoded umlauts in test output: Failed to read test report file ...\TEST-com.sos.scheduler.engine.plugins.webservice.services.EventsServiceIT.xml
//            // org.dom4j.DocumentException: Invalid byte 2 of 3-byte UTF-8 sequence. Nested exception: Invalid byte 2 of 3-byte UTF-8 sequence.
//    provideUmlautJob()
//    val responseString = postCommand("<show_state/>")
//    val jobAttributeValues = xml.XML.loadString(responseString) \ "answer" \ "state" \ "jobs" \ "job" map { o ⇒ (o \ "@job").text }
//    assert(jobAttributeValues.toSet == Set("a", "test-umlauts-äöüßÄÖÜ"))
//  }

  def postCommand(command: String): String =
    webResource.path("/jobscheduler/engine/command").accept(TEXT_XML_TYPE).`type`(TEXT_XML_TYPE).post(classOf[String], command)

  def getCommand(command: String): String = {
    val c = command.replaceAll(" ", "%20")
    get[String](normalizeUri(s"/jobscheduler/engine/command?command=$c"), Accept = List(TEXT_XML_TYPE))
  }
}

private object CommandServiceIT {
  private val StrippedModifyingCommand = "check_folders"
  private val ModifyingCommand = s"<$StrippedModifyingCommand/>"

  private def interceptHttpError(status: ClientResponse.Status)(body: ⇒ Unit): Unit = {
    intercept[UniformInterfaceException](body).getResponse.getStatus shouldEqual status.getStatusCode
  }
}
