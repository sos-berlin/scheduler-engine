package com.sos.scheduler.engine.tests.jira.js1163

import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichPath
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.taskserver.task.process.RichProcess
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1163.JS1163IT._
import java.nio.file.Files
import java.nio.file.Files.delete
import java.time.Instant
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
 * JS-1163 &lt;kill_task timeout="..."> with SIGTERM before SIGKILL.
 * <p>
 * JS-1307 SIGTERM on shell task with monitor is forwarded to shell process.
 * <p>
 * JS-1420 SIGTERM on shell task without monitor on classic agent is forwarded to shell process
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1163IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))
  private lazy val testFile = Files.createTempFile("test-", ".tmp")
  private lazy val killScriptFile = RichProcess.OS.newTemporaryShellFile("TEST") sideEffect {
    _.contentString = if (isWindows) s"echo KILL-ARGUMENTS=%* >$testFile\n" else "echo $* >$testFile\n"
  }
  onClose {
    delete(testFile)
    delete(killScriptFile)
  }
  override protected lazy val agentConfiguration = AgentConfiguration.forTest().copy(killScriptFile = Some(killScriptFile))
  private var results: Map[JobPath, TaskResult] = null
  private var killTime: Instant = null

  "kill_task with timeout but without immediately=true is rejected" in {
    val run = runJobFuture(TestJobPath)
    interceptSchedulerError(MessageCode("SCHEDULER-467")) {
      awaitSuccess(run.started)
      scheduler executeXml <kill_task job={TestJobPath.string} id={run.taskId.string} timeout="3"/>
    }
  }

  "SIGKILL" - {
    val settings = List(
      ("Without agent", { () ⇒ None }),
      ("With Universal Agent", { () ⇒ Some(agentUri) }),
      ("With TCP classic agent", { () ⇒ Some(s"127.0.0.1:$tcpPort")}))
    for ((testVariantName, agentAddressOption) ← settings) {
      testVariantName - {
        val jobPaths = List(StandardJobPath, StandardMonitorJobPath, ApiJobPath)
        s"(preparation: run and kill tasks)" in {
          deleteAndWriteConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = agentAddressOption().toList))
          controller.toleratingErrorCodes(Set("Z-REMOTE-101", "Z-REMOTE-122", "ERRNO-32", "WINSOCK-10053", "WINSOCK-10054", "SCHEDULER-202", "SCHEDULER-279", "SCHEDULER-280") map MessageCode) {
            val runs = jobPaths map { runJobFuture(_) }
            awaitSuccess(Future.sequence(runs map {_.started}))
            // Now, during slow Java start, shell scripts should have executed their "trap" commands
            sleep(1.s)
            killTime = now()
            for (run ← runs) scheduler executeXml <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true"/>
            results = awaitResults(runs map {_.result}) toKeyedMap {_.jobPath}
          }
        }

        for (jobPath ← jobPaths) {
          s"$jobPath - SIGKILL directly aborted process" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration
          }
        }
      }
    }
  }

  if (isWindows) {
    addWindowsTests()
  } else {
    addUnixTests()
  }

  private def addWindowsTests(): Unit = {
    "kill_task with timeout on Windows is rejected" in {
      val run = runJobFuture(WindowsJobPath)
      interceptSchedulerError(MessageCode("SCHEDULER-490")) {
        awaitSuccess(run.started)
        scheduler executeXml <kill_task job={WindowsJobPath.string} id={run.taskId.string} immediately="true" timeout="3"/>
      }
    }
  }

  private def addUnixTests(): Unit = {
    val settings = List(
      ("Without agent", true, { () ⇒ None }),
      ("With Universal Agent", false, { () ⇒ Some(agentUri) }),
      ("With TCP classic agent", true, { () ⇒ Some(s"127.0.0.1:$tcpPort")}))
    for ((testVariantName, monitorForwardsSignal, agentAddressOption) ← settings) {
      // monitorForwardsSignal: Universal Agent monitor does not forward signal to shell process!!!
      testVariantName - {
        s"(preparation: run and kill tasks)" in {
          deleteAndWriteConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = agentAddressOption().toList))
          controller.toleratingErrorCodes(Set("Z-REMOTE-101", "ERRNO-32", "SCHEDULER-202", "SCHEDULER-279", "SCHEDULER-280") map MessageCode) {
            val jobPaths = List(
              StandardJobPath, StandardMonitorJobPath,
              TrapJobPath, TrapMonitorJobPath,
              IgnoringJobPath, IgnoringMonitorJobPath,
              ApiJobPath)
            val runs = jobPaths map { runJobFuture(_) }
            awaitSuccess(Future.sequence(runs map { _.started }))
            // Now, during slow Java start, shell scripts should have executed their "trap" commands
            sleep(1.s)
            killTime = now()
            for (run ← runs) scheduler executeXml
                <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true" timeout={KillTimeout.getSeconds.toString}/>
            results = awaitResults(runs map { _.result }) toKeyedMap { _.jobPath }
          }
        }

        for (jobPath ← List(StandardJobPath) ++ monitorForwardsSignal.option(StandardMonitorJobPath)) {
          s"$jobPath - Without trap, SIGTERM directly aborts process" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration
          }
        }

        for (jobPath ← List(TrapJobPath) ++ monitorForwardsSignal.option(TrapMonitorJobPath)) {
          s"$jobPath - With SIGTERM trapped, SIGTERM aborts process after signal was handled" in {
            results(jobPath).logString should (not include FinishedNormally and include(SigtermTrapped))
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration + TrapDuration
          }
        }

        for (jobPath ← List(IgnoringJobPath) ++ monitorForwardsSignal.option(IgnoringMonitorJobPath)) {
          s"$jobPath - With SIGTERM ignored, timeout take effect" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).endedInstant should be > killTime + KillTimeout - 1.s
            results(jobPath).endedInstant should be < killTime + MaxKillDuration + KillTimeout
            results(jobPath).duration should be < UndisturbedDuration
          }
        }

        if (monitorForwardsSignal) {
          for ((jobPath, expectedSpoolerProcessResult) ← List(StandardMonitorJobPath → false, TrapMonitorJobPath → true/*, IgnoringMonitorJobPath → true ???*/)) {
            s"$jobPath - spooler_process_after was called" in {
              results(jobPath).logString should include(TestMonitor.spoolerProcessAfterString(expectedSpoolerProcessResult))
            }
          }
        }

        s"$testVariantName - API job has been directly aborted" in {
          results(ApiJobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
          results(ApiJobPath).endedInstant should be < killTime + 2.s
        }
      }
    }
  }

  "Universal Agent kill script called" in {
    logger.info(testFile.contentString)
    assert(testFile.contentString contains s"KILL-ARGUMENTS=-kill-agent-task-id=")
  }
}

private object JS1163IT {
  private val logger = Logger(getClass)
  private val TestJobPath = JobPath("/test")
  private val WindowsJobPath = JobPath("/test-windows")
  private val StandardJobPath = JobPath("/test-standard")
  private val StandardMonitorJobPath = JobPath("/test-standard-monitor")
  private val TrapJobPath = JobPath("/test-trap")
  private val TrapMonitorJobPath = JobPath("/test-trap-monitor")
  private val IgnoringJobPath = JobPath("/test-ignore")
  private val IgnoringMonitorJobPath = JobPath("/test-ignore-monitor")
  private val ApiJobPath = JobPath("/test-api")
  private val TestProcessClassPath = ProcessClassPath("/test")

  private val KillTimeout = 4.s
  private val MaxKillDuration = 2.s
  private val TrapDuration = 2.s  // Trap sleeps 2s
  private val UndisturbedDuration = 15.s

  private val SigtermTrapped = "SIGTERM HANDLED"
  private val FinishedNormally = "FINISHED NORMALLY"
}
