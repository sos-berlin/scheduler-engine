package com.sos.scheduler.engine.tests.jira.js1163

import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1163.JS1163IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * Tickets JS-1163, JS-1307.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1163IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val List(httpPort, tcpPort) = findRandomFreeTcpPorts(2)
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-http-port=$httpPort", s"-tcp-port=$tcpPort"))
  private var results: Map[JobPath, TaskResult] = null

  "kill_task with timeout but without immediately=true is rejected" in {
    val run = runJobFuture(TestJobPath)
    interceptSchedulerError(MessageCode("SCHEDULER-467")) {
      awaitSuccess(run.started)
      scheduler executeXml <kill_task job={TestJobPath.string} id={run.taskId.string} timeout="3"/>
    }
  }

  if (isWindows) {
    "kill_task with timeout on Windows is rejected" in {
      val run = runJobFuture(WindowsJobPath)
      interceptSchedulerError(MessageCode("SCHEDULER-490")) {
        awaitSuccess(run.started)
        scheduler executeXml <kill_task job={WindowsJobPath.string} id={run.taskId.string} immediately="true" timeout="3"/>
      }
    }
  }
  else {
    val settings = List(
      "Without agent" → { () ⇒ None },
      "With agent" → { () ⇒ Some(s"http://127.0.0.1:$httpPort") })
    for ((testVariantName, agentAddressOption) ← settings) {

      s"$testVariantName - (Run and kill tasks)" in {
        scheduler executeXml testProcessClass(agentAddressOption())

        controller.toleratingErrorCodes(Set("Z-REMOTE-101", "ERRNO-32", "SCHEDULER-202", "SCHEDULER-279", "SCHEDULER-280") map MessageCode) {
          val jobPaths = List(
            StandardJobPath, StandardMonitorJobPath,
            TrapJobPath, TrapMonitorJobPath,
            IgnoringJobPath, IgnoringMonitorJobPath,
            ApiJobPath)
          val runs = jobPaths map runJobFuture
          for (run ← runs) awaitSuccess(run.started)
          sleep(ShellStartupTime)  // Wait until shell scripts should have executed their "trap" commands
          for (run ← runs) scheduler executeXml
              <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true" timeout={KillTimeoutString}/>
          results = awaitResults(runs map { _.result }) toKeyedMap { _.jobPath }
        }
      }

      for (jobPath ← List(StandardJobPath, StandardMonitorJobPath)) {
        s"$testVariantName - $jobPath - Without trap, SIGTERM directly aborted process" in {
          results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
          results(jobPath).duration should be < KilledImmediatelyMaxDuration
        }
      }

      for (jobPath ← List(TrapJobPath, TrapMonitorJobPath)) {
        s"$testVariantName - $jobPath - With SIGTERM trapped, SIGTERM aborted process after signal was handled" in {
          results(jobPath).logString should (not include FinishedNormally and include (SigtermTrapped))
          results(jobPath).duration should be < KilledTrappedMaxDuration
        }
      }

      for (jobPath ← List(IgnoringJobPath, IgnoringMonitorJobPath)) {
        s"$testVariantName - $jobPath - With SIGTERM ignored, timeout took effect" in {
          results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
          results(jobPath).duration should (be >= KillTimeout and be < SleepDuration)
        }
      }

      s"$testVariantName - API job was aborted directly" in {
        results(ApiJobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
        results(ApiJobPath).duration should be < KilledImmediatelyMaxDuration
      }
    }

    s"TCP connected agent is not supported" in {
      scheduler executeXml <process_class replace="true" name="test" remote_scheduler={s"127.0.0.1:$tcpPort"}/>
      interceptSchedulerError(MessageCode("SCHEDULER-468")) {  // "Using this call is not possible in this context [kill timeout] [TCP based agent connection - please connect agent with HTTP]"
        val run = runJobFuture(StandardJobPath)
        awaitSuccess(run.started)
        scheduler executeXml <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true" timeout={KillTimeout.getStandardSeconds.toString}/>
      }
    }
  }
}

private object JS1163IT {
  private val TestJobPath = JobPath("/test")
  private val WindowsJobPath = JobPath("/test-windows")
  private val StandardJobPath = JobPath("/test-standard")
  private val StandardMonitorJobPath = JobPath("/test-standard-monitor")
  private val TrapJobPath = JobPath("/test-trap")
  private val TrapMonitorJobPath = JobPath("/test-trap-monitor")
  private val IgnoringJobPath = JobPath("/test-ignore")
  private val IgnoringMonitorJobPath = JobPath("/test-ignore-monitor")
  private val ApiJobPath = JobPath("/test-api")

  private val ShellStartupTime = 5.s
  private val KillTimeout = 9.s
  private val KillTimeoutString = (KillTimeout - ShellStartupTime).getStandardSeconds.toString
  private val KilledImmediatelyMaxDuration = 8.s
  private val KilledTrappedMaxDuration = 11.s
  private val SleepDuration = 13.s

  private val SigtermTrapped = "SIGTERM HANDLED"
  private val FinishedNormally = "FINISHED NORMALLY"

  private def testProcessClass(agentAddress: Option[String]) =
      <process_class replace="true" name="test" remote_scheduler={agentAddress.orNull}/>
}
