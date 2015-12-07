package com.sos.scheduler.engine.tests.jira.js1529

import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1529.JS1529IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1529IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"))

  private lazy val processClassConfigurations = List(
    ("without Agent", () ⇒ ProcessClassConfiguration()),
    ("with Universal Agent", () ⇒ ProcessClassConfiguration(agentUris = List(agentUri))),
    ("with Classic Agent", () ⇒ ProcessClassConfiguration(agentUris = List(s"127.0.0.1:$tcpPort"))))

  for ((name, config) ← processClassConfigurations) s"Job.state_text $name" in {
    deleteAndWriteConfigurationFile(ProcessClassPath("/test"), ProcessClassConfiguration())
    runJobAndWaitForEnd(TestJobPath, Map("A" → name))
    val expected = s"JOB STATE TEXT $name"
    assert(job(TestJobPath).stateText == expected)
    val answer = (scheduler executeXml <show_job job={TestJobPath.string}/>).answer
    assert((answer \ "job" \ "@state_text").toString == expected)
  }
}

private object JS1529IT {
  private val TestJobPath = if (isWindows) JobPath("/test-windows") else JobPath("/test-unix")
}
