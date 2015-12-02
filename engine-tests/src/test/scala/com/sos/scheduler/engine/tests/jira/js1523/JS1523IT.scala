package com.sos.scheduler.engine.tests.jira.js1523

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.http.client.heartbeat.HttpHeartbeatTiming
import com.sos.scheduler.engine.test.ImplicitTimeout
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1523 Agent with HTTP heartbeat.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1523IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private val delay = 2 * (HttpHeartbeatTiming.Default.timeout + 5.s)

  s"Agent with client-side HTTP heartbeat, delay_spooler_process = ${delay.pretty}" in {
    // While between two spooler_process(), two client-side heartbeats are sent to the Agent, keeping the task alive
    implicit val implicitTimeout = ImplicitTimeout(delay + 10.s)
    runJobAndWaitForEnd(JobPath("/test"), Map("delay" → delay.getSeconds.toString))
  }
}
