package com.sos.scheduler.engine.tests.jira.js1163

import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.base.process.ProcessSignal.{SIGKILL, SIGTERM}
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichPath
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ModifyJobCommand.Cmd.Unstop
import com.sos.scheduler.engine.data.xmlcommands.{ModifyJobCommand, ProcessClassConfiguration}
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.taskserver.task.process.Processes.newTemporaryShellFile
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
 * <p>
 * JS-1496 Universal Agent task server forwards SIGTERM to shell process, bypassing monitor
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1163IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))
  private lazy val testFile = Files.createTempFile("test-", ".tmp")
  private lazy val killScriptFile = newTemporaryShellFile("TEST") sideEffect { file ⇒ file.contentString =
    if (isWindows) s"echo KILL-ARGUMENTS=%* >>$testFile\n"
    else JavaResource("com/sos/scheduler/engine/tests/jira/js1163/kill-script.sh").asUTF8String concat s"\necho KILL-ARGUMENTS=$$arguments >>$testFile\n"  // echo only if script succeeds
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
            awaitSuccess(Future.sequence(runs map { _.started }))
            // Now, during slow Java start, shell scripts should have executed their "trap" commands
            sleep(1.s)
            killTime = now()
            for (run ← runs) scheduler executeXml <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true"/>
            results = awaitResults(runs map { _.result }) toKeyedMap { _.jobPath }
          }
        }

        for (jobPath ← jobPaths) {
          s"$jobPath - SIGKILL directly aborted process" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration
            assert(job(jobPath).state == JobState.stopped)
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

        if (agentAddressOption() contains agentUri) {
          "Kill script called" in {
            logger.info(testFile.contentString)
            val lines = testFile.contentString split "\n"
            assert((lines count { _ contains s"KILL-ARGUMENTS=-kill-agent-task-id=" }) == jobPaths.size)
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
      ("Without agent", { () ⇒ None }),
      ("With Universal Agent", { () ⇒ Some(agentUri) }),
      ("With TCP classic agent", { () ⇒ Some(s"127.0.0.1:$tcpPort")}))
    for ((testVariantName, agentAddressOption) ← settings) {
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

        for (jobPath ← List(StandardJobPath, StandardMonitorJobPath)) {
          s"$jobPath - Without trap, SIGTERM directly aborts process" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration
            results(jobPath).returnCode.normalized shouldEqual ReturnCode(SIGTERM)
            assert(job(jobPath).state == JobState.stopped)
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

        for (jobPath ← List(TrapJobPath, TrapMonitorJobPath)) {
          s"$jobPath - With SIGTERM trapped, SIGTERM aborts process after signal was handled" in {
            results(jobPath).logString should (not include FinishedNormally and include(SigtermTrapped))
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration + TrapDuration
            results(jobPath).returnCode shouldEqual ReturnCode(7)
            assert(job(jobPath).state == JobState.stopped)
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

        for (jobPath ← List(IgnoringJobPath, IgnoringMonitorJobPath)) {
          s"$jobPath - With SIGTERM ignored, timeout take effect" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).endedInstant should be > killTime + KillTimeout - 1.s
            results(jobPath).endedInstant should be < killTime + MaxKillDuration + KillTimeout
            results(jobPath).duration should be < UndisturbedDuration
            // Why not this ??? results(jobPath).returnCode.normalized shouldEqual ReturnCode(SIGKILL)
            results(jobPath).returnCode.normalized shouldEqual (jobPath match {
              case IgnoringJobPath if agentAddressOption() == Some(agentUri) ⇒ ReturnCode(1)  // Warum nicht auch SIGKILL ???
              case IgnoringJobPath ⇒ ReturnCode(SIGKILL)
              case IgnoringMonitorJobPath ⇒ ReturnCode(1)   // Warum nicht auch SIGKILL ???
            })
            assert(job(jobPath).state == JobState.stopped)
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

          for ((jobPath, expectedSpoolerProcessResult) ← List(StandardMonitorJobPath → false, TrapMonitorJobPath → false/*, IgnoringMonitorJobPath → true ???*/)) {
          s"$jobPath - spooler_process_after was called" in {
            results(jobPath).logString should include(TestMonitor.spoolerProcessAfterString(expectedSpoolerProcessResult))
          }
        }

        s"$testVariantName - API job has been directly aborted" in {
          results(ApiJobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
          results(ApiJobPath).endedInstant should be < killTime + 2.s
        }
      }
    }
  }
}

private[js1163] object JS1163IT {
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
  private[js1163] val UndisturbedDuration = 15.s

  private val SigtermTrapped = "SIGTERM HANDLED"
  private val FinishedNormally = "FINISHED NORMALLY"
}
