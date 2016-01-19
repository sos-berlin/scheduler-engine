package com.sos.scheduler.engine.tests.jira.js1517

import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1517IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {
  private lazy val List(tcpPort, httpPort) = findRandomFreeTcpPorts(2)
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort", s"-http-port=$httpPort"))

  List(
    "Without Agent" → { () ⇒ ProcessClassConfiguration(agentUris = List("")) },
    "With Universal Agent" → { () ⇒ ProcessClassConfiguration(agentUris = List(agentUri)) },
    "With TCP C++ Agent" → { () ⇒ ProcessClassConfiguration(agentUris = List(s"127.0.0.1:$tcpPort")) })
  .foreach { case (testGroupName, lazyProcessClassConfig) ⇒
    testGroupName in {
      deleteAndWriteConfigurationFile(ProcessClassPath("/test"), lazyProcessClassConfig())
      val result = runJobAndWaitForEnd(JobPath("/test"), Map("a" → "test"))
      assert(result.logString contains "TEST_VARIABLE_A=test")
    }
  }
}
