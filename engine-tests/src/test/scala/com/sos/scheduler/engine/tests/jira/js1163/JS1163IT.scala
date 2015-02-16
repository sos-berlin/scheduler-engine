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
      val run = runJobFuture(StandardJobPath)
      interceptSchedulerError(MessageCode("SCHEDULER-490")) {
        awaitSuccess(run.started)
        scheduler executeXml <kill_task job={StandardJobPath.string} id={run.taskId.string} immediately="true" timeout="3"/>
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

        controller.toleratingErrorCodes(Set(MessageCode("Z-REMOTE-101"), MessageCode("SCHEDULER-202"), MessageCode("SCHEDULER-279"), MessageCode("SCHEDULER-280"))) {
          val jobPaths = List(
            StandardJobPath, StandardMonitorJobPath,
            TerminatingJobPath, TerminatingMonitorJobPath,
            IgnoringJobPath, IgnoringMonitorJobPath,
            ApiJobPath)
          val runs = jobPaths map runJobFuture
          for (run ← runs) {
            awaitSuccess(run.started)
            scheduler executeXml <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true" timeout={KillTimeout.getStandardSeconds.toString}/>
          }
          results = awaitResults(runs map { _.result }) toKeyedMap { _.jobPath }
        }
      }

      s"$testVariantName - Without trap, SIGTERM directly aborted process" in {
        results(StandardJobPath).duration should be < KilledImmediatelyMaxDuration
        results(StandardJobPath).logString should not (include (FinishedNormallystring) or include (SigtermHandledString))
      }

      s"$testVariantName - With SIGTERM trapped, SIGTERM aborted process after signal was handled" in {
        results(TerminatingJobPath).duration should be < KilledImmediatelyMaxDuration
        results(TerminatingJobPath).logString should include (SigtermHandledString)
      }

      s"$testVariantName - With SIGTERM ignored, timeout took effect" in {
        results(IgnoringJobPath).duration should (be >= KillTimeout and be < SleepDuration)
        results(IgnoringJobPath).logString should not (include (FinishedNormallystring) or include (SigtermHandledString))
      }

      for (jobPath ← List(ApiJobPath, StandardMonitorJobPath, TerminatingMonitorJobPath, IgnoringMonitorJobPath)) {
        s"$testVariantName - $jobPath was aborted directly" in {
          results(jobPath).duration should be < KilledImmediatelyMaxDuration
          results(jobPath).logString should not (include (FinishedNormallystring) or include (SigtermHandledString))
        }
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
  private val StandardJobPath = JobPath("/test-standard")
  private val StandardMonitorJobPath = JobPath("/test-standard-monitor")
  private val TerminatingJobPath = JobPath("/test-terminate")
  private val TerminatingMonitorJobPath = JobPath("/test-terminate-monitor")
  private val IgnoringJobPath = JobPath("/test-ignore")
  private val IgnoringMonitorJobPath = JobPath("/test-ignore-monitor")
  private val ApiJobPath = JobPath("/test-api")

  private val KillTimeout = 7.s
  private val NeverTimeout = Int.MaxValue.s
  private val KilledImmediatelyMaxDuration = 6.s
  private val SleepDuration = 11.s

  private val SigtermHandledString = "SIGTERM HANDLED"
  private val FinishedNormallystring = "FINISHED NORMALLY"

  private def testProcessClass(agentAddress: Option[String]) =
      <process_class replace="true" name="test" remote_scheduler={agentAddress.orNull}/>
}
