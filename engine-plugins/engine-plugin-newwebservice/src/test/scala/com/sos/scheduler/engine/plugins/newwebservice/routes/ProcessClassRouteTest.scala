package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.jobscheduler.common.sprayutils.SprayUtils.pathSegments
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.event.Stamped
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.api.{ProcessClassClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.{ProcessClassDetailed, ProcessClassOverview, ProcessClassPath, ProcessClassView, ProcessDetailed}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.ProcessClassRouteTest._
import java.time.Instant
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.{ExecutionContext, Future}
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes.`application/json`
import spray.httpx.SprayJsonSupport._
import spray.httpx.marshalling.BasicMarshallers._
import spray.json.DefaultJsonProtocol._
import spray.routing.Route
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class ProcessClassRouteTest extends FreeSpec with ScalatestRouteTest with ProcessClassRoute {

  //private implicit lazy val actorSystem = ActorSystem("ProcessClassRouteTest")

  protected implicit def client = new ProcessClassClient with SchedulerOverviewClient {

    def agentUris = Future.successful(Stamped(1,
      Set(AgentAddress("http://agent-1.example.com"), AgentAddress("http://agent-2.example.com"))))

    def processClasses[V <: ProcessClassView: ProcessClassView.Companion](query: PathQuery) =
      implicitly[ProcessClassView.Companion[V]] match {

        case ProcessClassOverview ⇒ Future.successful(Stamped(1,
          query match {
            case PathQuery.All ⇒ List(
              AProcessClassOverview.asInstanceOf[V],
              BProcessClassOverview.asInstanceOf[V])
            case PathQuery.FolderTree(FolderPath("/A")) ⇒ List(
              AProcessClassOverview.asInstanceOf[V])
            case _ ⇒ fail()
          }))
        case _ ⇒ fail()
      }

    def processClass[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath) =
      implicitly[ProcessClassView.Companion[V]] match {
        case ProcessClassOverview ⇒
          assert(processClassPath == AProcessClassOverview.path)
          Future.successful(Stamped(2, AProcessClassOverview.asInstanceOf[V]))
        case ProcessClassDetailed ⇒
          assert(processClassPath == AProcessClassOverview.path)
          Future.successful(Stamped(2, AProcessClassDetailed.asInstanceOf[V]))
      }

    def overview = throw new NotImplementedError
  }

  protected implicit def webServiceContext = new WebServiceContext()

  protected implicit def executionContext = ExecutionContext.global

  private def route: Route =
    pathSegments("api/processClass") {
      processClassRoute
    }

  "GET" - {
    for (uri ← List(s"$ProcessClassUri/?return=ProcessClassOverview")) {
      s"$uri" in {
        Get(uri) ~> Accept(`application/json`) ~> route ~> check {
          val snapshot = responseAs[Stamped[List[ProcessClassOverview]]]
          assert(snapshot == Stamped(1, List(AProcessClassOverview, BProcessClassOverview)))
        }
      }
    }

    for (uri ← List(s"$ProcessClassUri/A/?return=ProcessClassOverview")) {
      s"$uri" in {
        Get(uri) ~> Accept(`application/json`) ~> route ~> check {
          val snapshot = responseAs[Stamped[List[ProcessClassOverview]]]
          assert(snapshot == Stamped(1, List(AProcessClassOverview)))
        }
      }
    }

    for (uri ← List(s"$ProcessClassUri/A/PROCESS-CLASS?return=ProcessClassDetailed")) {
      s"$uri" in {
        Get(uri) ~> Accept(`application/json`) ~> route ~> check {
          val snapshot = responseAs[Stamped[ProcessClassDetailed]]
          assert(snapshot == Stamped(2, AProcessClassDetailed))
        }
      }
    }
  }

  s"POST $ProcessClassUri" - {
    "All implicit" in {
      postJson(s"$ProcessClassUri?return=ProcessClassOverview", "{}") ~> route ~> check {
        val snapshot = responseAs[Stamped[List[ProcessClassOverview]]]
        assert(snapshot == Stamped(1, List(AProcessClassOverview, BProcessClassOverview)))
      }
    }

    "All explicit" in {
      val query = """{
          "path": "/"
        }"""
      postJson(s"$ProcessClassUri?return=ProcessClassOverview", query) ~> route ~> check {
        val snapshot = responseAs[Stamped[List[ProcessClassOverview]]]
        assert(snapshot == Stamped(1, List(AProcessClassOverview, BProcessClassOverview)))
      }
    }

    "Folder" in {
      val query = """{
          "path": "/A/"
        }"""
      postJson(s"$ProcessClassUri?return=ProcessClassOverview", query) ~> route ~> check {
        val snapshot = responseAs[Stamped[List[ProcessClassOverview]]]
        assert(snapshot == Stamped(1, List(AProcessClassOverview)))
      }
    }

    "A single process class" in {
      val query = """{
          "path": "/A/PROCESS-CLASS"
        }"""
      postJson(s"$ProcessClassUri?return=ProcessClassOverview", query) ~> route ~> check {
        val snapshot = responseAs[Stamped[ProcessClassOverview]]
        assert(snapshot == Stamped(2, AProcessClassOverview))
      }
    }

    def postJson(uri: String, query: String) = Post(uri, query)(stringMarshaller(`application/json`)) ~> Accept(`application/json`)
  }
}

object ProcessClassRouteTest {
  private val ProcessClassUri = "/api/processClass"

  private val AProcessClassOverview = ProcessClassOverview(
    ProcessClassPath("/A/PROCESS-CLASS"),
    FileBasedState.active,
    processLimit = 10,
    usedProcessCount = 1,
    obstacles = Set())

  private val AProcessClassDetailed = ProcessClassDetailed(
    overview = AProcessClassOverview,
    selectionMethod = "FixedPriority",
    List(
      AgentAddress("https://example.com:4445")),
    List(
      ProcessDetailed(
        JobPath("/JOB"),
        TaskId(333),
        Instant.parse("2016-10-26T11:22:33.444Z"),
        Some(4444),
        Some(AgentAddress("http://AGENT")))))

  private val BProcessClassOverview = ProcessClassOverview(
    ProcessClassPath("/B/PROCESS-CLASS"),
    FileBasedState.active,
    processLimit = 20,
    usedProcessCount = 19,
    obstacles = Set())
}
