package com.sos.scheduler.engine.tests.jira.js1666

import com.sos.scheduler.engine.agent.data.web.AgentUris
import com.sos.scheduler.engine.agent.views.AgentOverview
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.common.WebError
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
  private lazy val agentUris = AgentUris(agent.localUri)

  "AgentUris" in {
    assert((webSchedulerClient.agentUris await TestTimeout).value == Set(agent.localUri))
  }

  "Forbidden Agent" in {
    val alienAgentUris = AgentUris("http://example.com:5555")
    val e = intercept[UnsuccessfulResponseException] {
      webSchedulerClient.agentGet[AgentOverview](alienAgentUris.overview) await TestTimeout
    }
    assert(e.response.status == BadRequest)
    val Right(webError) = e.response.as[WebError]
    assert(webError.message == "Unknown Agent")
  }

  "Forbidden path" in {
    val e = intercept[UnsuccessfulResponseException] {
      webSchedulerClient.agentGet[AgentOverview](s"${agent.localUri}/jobscheduler/FORBIDDEN") await TestTimeout
    }
    assert(e.response.status == Forbidden)
    val Right(webError) = e.response.as[WebError]
    assert(webError.message == "Forbidden Agent URI: /jobscheduler/FORBIDDEN")
  }

  "AgentOverview" in {
    val expectedAgentOverview = agentClient.get[AgentOverview](_.overview) await 10.s
    val agentOverview = webSchedulerClient.agentGet[AgentOverview](agentUris.overview) await TestTimeout
    assert(agentOverview == expectedAgentOverview.copy(system = agentOverview.system, java = agentOverview.java))
  }
}
