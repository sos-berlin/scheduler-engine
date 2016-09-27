package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClient
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.agent.views.AgentOverview
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import spray.http.HttpHeaders._
import spray.http.MediaTypes._
import spray.http.StatusCodes.{Forbidden, OK}
import spray.httpx.SprayJsonSupport._
import spray.routing.Directives._
import spray.routing.ValidationRejection
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class AgentRouteTest extends FreeSpec with BeforeAndAfterAll with HasCloser with ScalatestRouteTest with AgentTest with AgentRoute {

  private implicit lazy val actorSystem = ActorSystem("OrderRoute")

  private lazy val agentClient = new AgentClient.Standard(agent.localUri)
  protected def actorRefFactory = actorSystem
  protected def executionContext = actorSystem.dispatcher
  protected def agentClients = List(agentClient)

  override protected def afterAll() = {
    close()
    actorSystem.shutdown()
    super.afterAll()
  }

  private def route =
    compressResponseIfRequested(()) {
      pathPrefix("api" / "agent") {
        agentRoute
      }
    }

  "Unknown agent" in {
    Get(s"/api/agent/https://unknown:5555/jobscheduler/agent/api") ~> Accept(`application/json`) ~> route ~> check {
      assert(!handled)
      assert(rejection == ValidationRejection("Unknown Agent"))
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

  "Test" in {
    Get(s"/api/agent/${agent.localUri}/jobscheduler/agent/api") ~> Accept(`application/json`) ~> route ~> check {
      val directAgentOverview = agentClient.get[AgentOverview](_.overview) await 10.s
      println(headers)
      if (status == OK) {
        val agentOverview = responseAs[AgentOverview].copy(system = directAgentOverview.system, java = directAgentOverview.java)
        assert(agentOverview == directAgentOverview)
      } else {
        println(status)
        fail(responseAs[String])
      }
    }
  }
}
