package com.sos.scheduler.engine.tests.jira.js1393

import com.sos.jobscheduler.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.util.matching.Regex

/**
 * JS-1393 Identify output channel in JobScheduler logs.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1393IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  "Identify output channel in JobScheduler logs" - {
    "without Agent" in {
      writeConfigurationFile(ProcessClassPath("/test"), ProcessClassConfiguration())
      testOutput()
    }

    "with Agent" in {
      deleteAndWriteConfigurationFile(ProcessClassPath("/test"), ProcessClassConfiguration(agentUris = List(agentUri)))
      testOutput()
    }
  }

  private def testOutput(): Unit = {
    val log = runJob(JobPath("/test")).logString
    val emptyEcho = if (isWindows) "\"\"" else ""
    selectStdLines(log, "[stdout]").toSet shouldEqual
      Set(emptyEcho, "out out out out out out out out out out out out out out out out out out out out out out out out out")
    selectStdLines(log, "[stderr]").toSet shouldEqual
      Set(emptyEcho, "err err err err err err err err err err err err err err err err err err err err err err err err err")

  }

  private def selectStdLines(log: String, channel: String) =
    log.lines filter { _ contains channel } map { line ⇒ (line split Regex.quote(channel))(1).trim }
}
