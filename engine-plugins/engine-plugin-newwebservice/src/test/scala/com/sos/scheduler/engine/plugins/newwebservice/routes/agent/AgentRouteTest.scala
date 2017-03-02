package com.sos.scheduler.engine.plugins.newwebservice.routes.agent

import akka.actor.ActorSystem
import com.sos.jobscheduler.agent.client.AgentClient
import com.sos.jobscheduler.agent.test.AgentTest
import com.sos.jobscheduler.agent.views.AgentOverview
import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.scalautil.HasCloser
import com.sos.jobscheduler.common.sprayutils.SprayUtils.pathSegments
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.event.{EventId, Stamped}
import com.sos.scheduler.engine.client.api.ProcessClassClient
import com.sos.scheduler.engine.data.processclass.ProcessClassView.Companion
import com.sos.scheduler.engine.data.processclass.{ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.agent.AgentRoute._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.Future
import spray.http.HttpHeaders._
import spray.http.MediaTypes._
import spray.http.StatusCodes.{BadRequest, Forbidden, OK}
import spray.http.Uri
import spray.httpx.SprayJsonSupport._
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class AgentRouteTest extends FreeSpec with BeforeAndAfterAll with HasCloser with ScalatestRouteTest with AgentTest with AgentRoute {

  private implicit lazy val actorSystem = ActorSystem("OrderRoute")

  private lazy val agentClient = AgentClient(agent.localUri.string)
  protected def isKnownAgentUriFuture(uri: AgentAddress) = Future.successful(uri == agent.localUri)
  protected val webServiceContext = new WebServiceContext()
  protected def actorRefFactory = actorSystem
  protected def executionContext = actorSystem.dispatcher
  protected val toAgentClient = (uri: AgentAddress) ⇒ AgentClient(uri.string)

  protected val client = new ProcessClassClient {
    def agentUris = Future.successful(Stamped(EventId(1), Set(agent.localUri)))

    def processClasses[V <: ProcessClassView: Companion](q: PathQuery) = throw new NotImplementedError

    def processClass[V <: ProcessClassView: Companion](processClassPath: ProcessClassPath) = throw new NotImplementedError
  }
  override protected def afterAll() = {
    close()
    actorSystem.shutdown()
    super.afterAll()
  }

  private def route =
    compressResponseIfRequested(()) {
      pathSegments("api/agent") {
        agentRoute
      }
    }

  "splitIntoAgentUriAndTail" in {
    assert(splitIntoAgentUriAndTail(Uri.Path.Empty, query = Uri.Query.Empty) == ((AgentAddress(""), Uri(""))))
    assert(splitIntoAgentUriAndTail(Uri.Path("http://host:5555/jobscheduler/agent/api"), query = Uri.Query("q" → "1")) ==
      ((AgentAddress("http://host:5555"), Uri("/jobscheduler/agent/api?q=1"))))
  }

  "isAllowedTailUri" in {
    assert(!isAllowedTailUri(Uri("")))
    assert(!isAllowedTailUri(Uri("/jobscheduler")))
    assert(isAllowedTailUri(Uri("/jobscheduler/agent/api")))
    assert(!isAllowedTailUri(Uri("/jobscheduler/agent/api/something")))
    assert(!isAllowedTailUri(Uri("/jobscheduler/agent/api?q=1")))
  }

  "List of Agents" in {
    Get(s"/api/agent/") ~> Accept(`application/json`) ~> route ~> check {
      assert(status == OK)
      assert(responseAs[Stamped[Set[AgentAddress]]] == (client.agentUris await 10.s))
    }
  }

  "Unknown agent" in {
    Get(s"/api/agent/https://unknown:5555/jobscheduler/agent/api") ~> Accept(`application/json`) ~> route ~> check {
      assert(status == BadRequest)
      assert(responseAs[String] == "Unknown Agent")
    }
  }

  "Forbidden URI" in {
    Get(s"/api/agent/https://unknown:5555/jobscheduler/secret") ~> Accept(`application/json`) ~> route ~> check {
      assert(status == Forbidden)
    }
    Get(s"/api/agent/https://unknown:5555/jobscheduler/agent/api/task/") ~> Accept(`application/json`) ~> route ~> check {
      assert(status == Forbidden)
    }
  }

  "Agent URI" in {
    Get(s"/api/agent/${agent.localUri}/jobscheduler/agent/api") ~> Accept(`application/json`) ~> route ~> check {
      val expectedAgentOverview = agentClient.get[AgentOverview](_.overview) await 10.s
      println(headers)
      if (status == OK) {
        val agentOverview = responseAs[AgentOverview]
        assert(agentOverview == expectedAgentOverview.copy(system = agentOverview.system, java = agentOverview.java))
      } else {
        println(status)
        fail(responseAs[String])
      }
    }
  }
}
