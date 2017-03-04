package com.sos.scheduler.engine.plugins.newwebservice.tests

import akka.actor.ActorSystem
import com.sos.jobscheduler.common.scalautil.Closers.implicits._
import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.jobscheduler.common.sprayutils.XmlString
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder._
import com.sos.jobscheduler.data.event.Stamped
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, FileBasedOverview, FileBasedState}
import com.sos.scheduler.engine.data.job.{JobDescription, JobPath}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.plugins.newwebservice.tests.NewWebServicePluginIT._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext
import spray.client.pipelining._
import spray.http.HttpHeaders.{Accept, Location}
import spray.http.HttpMethods._
import spray.http.MediaTypes.`text/html`
import spray.http.StatusCodes.{BadRequest, Forbidden, MethodNotAllowed, OK, TemporaryRedirect}
import spray.http.{StatusCode, Uri}
import spray.httpx.UnsuccessfulResponseException

/**
  * More tests in JS1642IT.
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class NewWebServicePluginIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val httpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-http-port=$httpPort"))
  private lazy val rootUri = s"http://127.0.0.1:$httpPort"
  protected lazy val client = new StandardWebSchedulerClient(rootUri).closeWithCloser
  private implicit lazy val actorSystem = ActorSystem("NewWebServicePluginIT") withCloser { _.shutdown() }
  private implicit lazy val executionContext = instance[ExecutionContext]

  import client.uris

  "HTTP methods" - {
    for ((method, expected) ← Array(GET → OK, HEAD → OK, TRACE → MethodNotAllowed, PUT → MethodNotAllowed, POST → MethodNotAllowed))
      s"$method -> $expected" in {
        val response = (new RequestBuilder(method).apply(uris.overview) ~> sendReceive) await TestTimeout
        assert(response.status == expected)
      }
  }

  "HTTP redirects for text/html" - {
    for (path ← Array(rootUri, s"$rootUri/", s"$rootUri/jobscheduler", s"$rootUri/jobscheduler/"))
      s"$GET $path" in {
        val response = (Get(path) ~> Accept(`text/html`) ~> sendReceive) await TestTimeout
        assert(response.status == TemporaryRedirect)
        assert(response.header[Location].get.value == "/jobscheduler/master")  // But "/jobscheduler/joc", if JOC 1 is installed
      }
  }

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
      interceptHttpError(Forbidden) {
        getCommand(command)
      }
    }
    httpGetShouldForbid(ModifyingCommand)
    httpGetShouldForbid(StrippedModifyingCommand)
    httpGetShouldForbid("<start_job job='/a'/>")
  }

  "GET rejects a concatenated modifying command" in {
    interceptHttpError(BadRequest) {
      getCommand(s"<show_state/>$ModifyingCommand")
    }
  }

  "Job description with umlauts" in {
    val descriptionString = "TEST-DESCRIPTION mit Ümläüten"
    assert(postCommand(s"<show_jobs what='description'/>") contains descriptionString)
    assert((client.job[JobDescription](JobPath("/test-umlauts")) await TestTimeout).value.description == descriptionString)
  }

  "FileBasedOverview" in {
    val Stamped(_, overview) = client.fileBased[ProcessClassPath, FileBasedOverview](ProcessClassPath.Default) await TestTimeout
    assert(overview == FileBasedOverview(ProcessClassPath.Default, FileBasedState.active))
  }

  "FileBasedOverviews" in {
    val Stamped(_, overviews) = client.fileBaseds[ProcessClassPath, FileBasedOverview](PathQuery.All) await TestTimeout
    assert(overviews == Vector(FileBasedOverview(ProcessClassPath.Default, FileBasedState.active)))
  }

  "FileBasedDetailed" in {
    val Stamped(_, detailed) = client.fileBased[ProcessClassPath, FileBasedDetailed](ProcessClassPath.Default) await TestTimeout
    assert(detailed.overview == FileBasedOverview(ProcessClassPath.Default, FileBasedState.active))
  }

  "FileBasedDetaileds" in {
    val Stamped(_, detaileds) = client.fileBaseds[ProcessClassPath, FileBasedDetailed](PathQuery.All) await TestTimeout
    assert((detaileds map { _.overview }) == Vector(FileBasedOverview(ProcessClassPath.Default, FileBasedState.active)))
  }

  "FileBasedSource" in {
    val x = client.fileBasedSourceXml[JobPath, XmlString](TestUmlautsJobPath) await TestTimeout
    assert(SafeXML.loadString(x.string) == testEnvironment.fileFromPath(TestUmlautsJobPath).xml)
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

  private def postCommand(command: String): String =
    client.uncheckedExecuteXml(command) await TestTimeout

  private def getCommand(command: String): String =
    (sendReceive ~> unmarshal[String]).apply(Get(Uri(s"$rootUri/jobscheduler/master/api/command").copy(query = Uri.Query("command" → command)))) await TestTimeout
}

private object NewWebServicePluginIT {
  private val StrippedModifyingCommand = "check_folders"
  private val ModifyingCommand = s"<$StrippedModifyingCommand/>"
  private val TestUmlautsJobPath = JobPath("/test-umlauts")

  private def interceptHttpError(status: StatusCode)(body: ⇒ Unit): Unit = {
    intercept[UnsuccessfulResponseException](body).response.status shouldEqual status
  }
}
