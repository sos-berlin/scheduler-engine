package com.sos.scheduler.engine.tests.jira.js1454.long

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1454.long.JS1454IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1454 Keep-alive packets for classic Agent TCP communication.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1454IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"),
    logCategories = s"$KeepAliveLogCategory")

  "Proper termination with long keep-alive duration" in {
    writeConfigurationFile(ProcessClassPath("/test-agent"),
      ProcessClassConfiguration(agentUris = List(AgentAddress(s"127.0.0.1:$tcpPort"))))
    runJob(JobPath("/test-sleep"))
    val schedulerLog = testEnvironment.schedulerLog.contentString
    assert(schedulerLog contains s"{$KeepAliveLogCategory} Stopped")
  }
}

private object JS1454IT {
  private val KeepAliveLogCategory = "object_server.keep_alive"
}
