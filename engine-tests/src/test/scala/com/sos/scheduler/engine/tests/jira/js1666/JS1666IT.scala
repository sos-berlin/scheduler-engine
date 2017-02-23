package com.sos.scheduler.engine.tests.jira.js1666

import com.sos.jobscheduler.agent.data.web.AgentUris
import com.sos.jobscheduler.agent.views.AgentOverview
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.jobscheduler.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder._
import com.sos.jobscheduler.data.event.EventId
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.http.StatusCodes.{BadRequest, Forbidden}
import spray.httpx.SprayJsonSupport._
import spray.httpx.UnsuccessfulResponseException
import spray.httpx.unmarshalling._
import spray.json.DefaultJsonProtocol._
import spray.json._

/**
  * JS-1666 JobScheduler web service forwards to Agent.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1666IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val httpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-http-port=$httpPort"))
  protected lazy val webSchedulerClient = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  private lazy val agentUris = AgentUris(agentUri)

  "WebSchedulerClient" - {
    "AgentUris" in {
      assert((webSchedulerClient.agentUris await TestTimeout).value == Set(agentUri))
    }

    "Forbidden Agent" in {
      val alienAgentUris = AgentUris("http://example.com:5555")
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.agentGet[AgentOverview](alienAgentUris.overview) await TestTimeout
      }
      assert(e.response.status == BadRequest)
      assert(e.response.as[String] == Right("Unknown Agent"))
    }

    "Forbidden path" in {
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.agentGet[AgentOverview](s"$agentUri/jobscheduler/FORBIDDEN") await TestTimeout
      }
      assert(e.response.status == Forbidden)
      assert(e.response.as[String] == Right("Forbidden Agent URI: /jobscheduler/FORBIDDEN"))
    }

    "AgentOverview" in {
      val expectedAgentOverview = agentClient.get[AgentOverview](_.overview) await 10.s
      val agentOverview = webSchedulerClient.agentGet[AgentOverview](agentUris.overview) await TestTimeout
      assert(agentOverview == expectedAgentOverview.copy(system = agentOverview.system, java = agentOverview.java))
    }
  }

  "JSON" - {
    "/api/agent/" in {
      val jsObject = webSchedulerClient.getByUri[JsObject]("api/agent/") await TestTimeout
      val eventId = jsObject.fields("eventId").convertTo[EventId]
      assert(jsObject ==
        s"""{
          "eventId": $eventId,
          "elements": [
            "$agentUri"
          ]
        }""".parseJson)
    }

    "/api/agent/FORBIDDEN-URI/jobscheduler/agent/api" in {
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.getByUri[String](s"api/agent/http://example.com:5555/jobscheduler/agent/api") await TestTimeout
      }
      assert(e.response.status == BadRequest)
      assert(e.response.as[String] == Right("Unknown Agent"))
    }

    "/api/agent/(agentUri)/jobscheduler/FORBIDDEN" in {
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.getByUri[String](s"api/agent/$agentUri/jobscheduler/FORBIDDEN") await TestTimeout
      }
      assert(e.response.status == Forbidden)
      assert(e.response.as[String] == Right("Forbidden Agent URI: /jobscheduler/FORBIDDEN"))
    }

    "Forbidden path" in {
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.agentGet[AgentOverview](s"$agentUri/jobscheduler/FORBIDDEN") await TestTimeout
      }
      assert(e.response.status == Forbidden)
      assert(e.response.as[String] == Right("Forbidden Agent URI: /jobscheduler/FORBIDDEN"))
    }

    "/api/agent/(agentUri)/jobscheduler/agent/api" in {
      val jsObject = webSchedulerClient.getByUri[JsObject](s"api/agent/$agentUri/jobscheduler/agent/api") await TestTimeout
      assert(jsObject.fields.keySet == Set(
        "version", "startedAt", "totalTaskCount", "currentTaskCount", "isTerminating", "system", "java"))
    }
  }
}
