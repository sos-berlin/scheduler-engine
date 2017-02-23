package com.sos.scheduler.engine.tests.jira.js1039

import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet
import com.sos.scheduler.engine.test.SchedulerTestUtils.{runJob, writeConfigurationFile}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1039.JS1039TaskStdoutIT._
import java.util.regex.Pattern
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.util.matching.Regex

/** JS-1039 FIXED: API functions stdout_text and stderr_text return empty strings when used in monitor of shell-job.
  * Prüft, ob Task.stdout_text und Task.stderr_text die Ausgaben vom Shell-Prozess enthalten
  * und ob die Ausgaben des Shell-Prozesses und des Monitors im Task-Log erscheinen.
  *
  * 32 Tests: Shell oder API, mit oder ohne Monitor, lokal oder fern, stdout oder stderr:
  * Skript schreibt, spooler_task_after() schreibt, spooler_task_after() liest spooler_task.stdxxx_text. */
@RunWith(classOf[JUnitRunner])
final class JS1039TaskStdoutIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"))
  private lazy val schedulerVariables = instance[SchedulerVariableSet]

  private lazy val jobResults: Map[JobPath, JobResult] = {
    (JobSettings map { _.jobPath } map { jobPath =>
      for (o <- StdOutErrList) schedulerVariables(o) = ""
      runJob(jobPath)
      jobPath -> JobResult(
        taskLog = controller.environment.taskLogFileString(jobPath),
        variableMap = schedulerVariables.toMap)
    }).toMap
  }

  protected override def onSchedulerActivated(): Unit = {
    writeConfigurationFile(ProcessClassPath("/test-remote"), ProcessClassConfiguration(agentUris = List(AgentAddress(s"127.0.0.1:$tcpPort"))))
    super.onSchedulerActivated()
  }

  checkJobs(s"Task log should contain output from script") { (result, outOrErr) =>
    shouldOccurExactlyOnce(in = result.taskLog, what = s"/script $outOrErr/")
  }

  checkJobs(s"Task log should contain output from spooler_task_after()", _.hasMonitor) { (result, outOrErr) =>
    shouldOccurExactlyOnce(in = result.taskLog, what = s"/spooler_task_after $outOrErr/")
  }

  checkJobs(s"In spooler_task_after(), spooler_task.stdxxx_text contains scripts output", _.hasMonitor) { (result, outOrErr) =>
    result.variableMap(outOrErr) should include (s"/script $outOrErr/")
  }

  private def checkJobs(testName: String, predicate: JobSetting => Boolean = _ => true)(f: (JobResult, String) => Unit): Unit = {
    testName - {
      for (jobSetting <- JobSettings if predicate(jobSetting)) {
        s"Job ${jobSetting.jobPath.name}" - {
          for (outOrErr <- StdOutErrList) {
            outOrErr.toLowerCase in {
              f(jobResults(jobSetting.jobPath), outOrErr)
            }
          }
        }
      }
    }
  }

  private def shouldOccurExactlyOnce(in: String, what: String): Unit = {
    in should include (what)
    withClue(s"'$what' should occur only once:") {
      (new Regex(Pattern.quote(what)) findAllIn in).size shouldEqual 1
    }
  }
}


private object JS1039TaskStdoutIT {

  private val StdOutErrList = List("STDOUT", "STDERR")  // Nach stdout und stderr geschriebene Strings und zugleich Namen globaler Scheduler-Variablen.

  private case class JobSetting(jobPath: JobPath, hasMonitor: Boolean = false)

  private case class JobResult(taskLog: String, variableMap: Map[String, String])

  private val JobSettings = List(
    JobSetting(JobPath("/test-local-shell")),
    JobSetting(JobPath("/test-local-shell-monitor"), hasMonitor = true),
    JobSetting(JobPath("/test-local-api")),
    JobSetting(JobPath("/test-local-api-monitor"), hasMonitor = true),
    JobSetting(JobPath("/test-remote-shell")),
    JobSetting(JobPath("/test-remote-shell-monitor"), hasMonitor = true),
    JobSetting(JobPath("/test-remote-api")),
    JobSetting(JobPath("/test-remote-api-monitor"), hasMonitor = true))
}
