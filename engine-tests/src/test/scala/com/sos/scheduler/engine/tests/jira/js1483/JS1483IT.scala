package com.sos.scheduler.engine.tests.jira.js1483

import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.soslicense.LicenseKeyParameterIsMissingException
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.data.job.{JobPath, JobState}
import com.sos.scheduler.engine.kernel.database.JdbcConnectionPool
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1483.JS1483IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1483 Task start failure due to missing license key is stated in Job.state_text.
 * JS-1482 Without a license key, Agent runs one task at a time
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1483IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  "Without a license key, Agent runs one task at a time - JS-1482" in {
    (for (_ ← 1 to 10) yield startJob(TestJobPath).result) await TestTimeout
  }

  if (sys.props contains "test.speed") "Speed test" in {
    val n = 1000
    val stopwatch = new Stopwatch
    (for (_ ← 1 to n) yield startJob(TestJobPath).result) await (n / 1000 + 1) * TestTimeout
    logger.info(stopwatch.itemsPerSecondString(n, "job"))
  }

  "Task start failure due to missing license key is stated in Job.state_text" in {
    // Test does not work with external license keys as in ~/sos.ini or /etc/sos.ini
    val firstRun = startJob(SleepJobPath)
    awaitSuccess(firstRun.started)
    controller.toleratingErrorCodes(_ ⇒ true) {
      val taskId = startJob(TestJobPath).taskId
      waitForCondition(TestTimeout, 100.ms) { jobOverview(TestJobPath).state == JobState.stopped }
      assert(job(TestJobPath).stateText startsWith classOf[LicenseKeyParameterIsMissingException].getSimpleName)
      assert(job(TestJobPath).stateText contains "No license key provided by master to execute jobs in parallel")

      injector.instance[JdbcConnectionPool].readOnly { connection ⇒
        val table = injector.instance[SchedulerConfiguration].jobHistoryTableName
        val sql = s"""select "ID", "JOB_NAME", "AGENT_URL", "ERROR" from $table where "ID"=?"""
        autoClosing(connection.prepareStatement(sql)) { stmt ⇒
          stmt.setInt(1, taskId.number)
          val resultSet = stmt.executeQuery()
          if (!resultSet.next()) fail(s"Missing record in $table for $taskId")
          logger.info(s"ID=${resultSet.getInt("ID")} JOB_NAME=${resultSet.getString("JOB_NAME")} ERROR=${resultSet.getBoolean("ERROR")} AGENT_URL=${resultSet.getString("AGENT_URL")}")
          assert(resultSet.getInt("ID") == taskId.number)
          assert(resultSet.getString("JOB_NAME") == TestJobPath.withoutStartingSlash)
          assert(resultSet.getBoolean("ERROR"))
          assert(resultSet.getString("AGENT_URL") == agentUri.string)
        }
      } await 99.s

      scheduler executeXml <kill_task job="/test-sleep" id={firstRun.taskId.string} immediately="true"/>
      awaitSuccess(firstRun.closed)
    }
  }

  if (isWindows) "JS-861 Windows: Unknown credential key" in {
    // After a start with an unknown credentials key, the Agent task should be removed and not counted to the licence limit.
    controller.toleratingErrorCodes( _ ⇒ true) {
      for (_ ← 1 to 10) yield
        startJob(JobPath("/test-invalid-credentials")).result await TestTimeout
    }
  }
}

private object JS1483IT {
  private val logger = Logger(getClass)
  private val TestJobPath = JobPath("/test")
  private val SleepJobPath = JobPath("/test-sleep")
}
