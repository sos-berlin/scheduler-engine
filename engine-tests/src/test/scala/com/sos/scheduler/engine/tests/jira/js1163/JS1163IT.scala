package com.sos.scheduler.engine.tests.jira.js1163

import com.sos.jobscheduler.agent.data.ProcessKillScript
import com.sos.jobscheduler.base.process.ProcessSignal
import com.sos.jobscheduler.base.process.ProcessSignal.{SIGKILL, SIGTERM}
import com.sos.jobscheduler.common.log.LazyScalaLogger.AsLazyScalaLogger
import com.sos.jobscheduler.common.process.Processes.newTemporaryShellFile
import com.sos.jobscheduler.common.scalautil.Collections.implicits._
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits.RichPath
import com.sos.jobscheduler.common.scalautil.Futures.implicits.RichFutures
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.scalautil.SideEffect._
import com.sos.jobscheduler.common.system.OperatingSystem.{isSolaris, isWindows}
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.Exceptions.ignoreException
import com.sos.jobscheduler.common.utils.{Exceptions, JavaResource}
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.job.ReturnCode
import com.sos.scheduler.engine.data.job.{JobPath, JobState}
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ModifyJobCommand.Cmd.Unstop
import com.sos.scheduler.engine.data.xmlcommands.{ModifyJobCommand, ProcessClassConfiguration}
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1163.JS1163IT._
import java.nio.file.Files.{createTempDirectory, delete}
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
 * <p>
 * JS-1468 -agent-task-id and kill script
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1163IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val killScriptCallsDir = createTempDirectory("kill-script-calls-")  // Contains an empty file for every call of kill script
  private lazy val killScriptFile = newTemporaryShellFile("TEST") sideEffect { _.contentString =
    if (isWindows) "@echo off\n" + s"""echo >"$killScriptCallsDir/%*"""" + "\n"
    else JavaResource("com/sos/scheduler/engine/tests/jira/js1163/kill-script.sh").asUTF8String +
      s"""touch "$killScriptCallsDir/$$arguments"""" + "\n"
  }
  onClose {
    killScriptCallsDir.pathSet foreach delete
    delete(killScriptCallsDir)
    delete(killScriptFile)
  }
  override protected def newAgentConfiguration() = super.newAgentConfiguration().copy(killScript = Some(ProcessKillScript(killScriptFile)))
  private var results: Map[JobPath, TaskResult] = null
  private var killTime: Instant = null

  private val universalAgentSetting = new UniversalAgentSetting(agentUri = () ⇒ agentUri)
  private val settings = List(NoAgentSetting, universalAgentSetting)

  "Universal Agent unregisters task after normal termination" in {
    requireNoTasksAreRegistered()
    awaitResults(for (jobPath ← List(JobPath("/test-short"), JobPath("/test-short-api"))) yield startJob(jobPath).closed)
    requireNoTasksAreRegistered()
  }

  "kill_task with timeout but without immediately=true is rejected" in {
    val run = startJob(TestJobPath)
    interceptSchedulerError(MessageCode("SCHEDULER-467")) {
      awaitSuccess(run.started)
      scheduler executeXml <kill_task job={TestJobPath.string} id={run.taskId.string} timeout="3"/>
    }
  }

  "SIGKILL" - {
    for (setting ← settings) {
      s"$setting" - {
        val jobPaths = List(StandardJobPath, StandardMonitorJobPath, ApiJobPath)
        s"(preparation: run and kill tasks)" in {
          deleteAndWriteConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = setting.agentUriOption.toList))
          //controller.toleratingErrorCodes(Set("Z-REMOTE-101", "Z-REMOTE-122", "ERRNO-32", "WINSOCK-10053", "WINSOCK-10054", "SCHEDULER-202", "SCHEDULER-279", "SCHEDULER-280") map MessageCode) {
          controller.toleratingErrorCodes(_ ⇒ true) {
            val runs = jobPaths map { startJob(_) }
            awaitSuccess(Future.sequence(runs map { _.started }))
            // Now, during slow Java start, shell scripts should have executed their "trap" commands
            sleep(1.s)
            killTime = now()
            for (run ← runs) scheduler executeXml <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true"/>
            results = awaitResults(runs map { _.result }) toKeyedMap { _.jobPath }
          }
        }

        for (jobPath ← jobPaths) {
          s"$jobPath - SIGKILL directly aborts process" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration
            assert(jobOverview(jobPath).state == JobState.stopped)
            val normalizedReturnCode = results(jobPath).returnCode.normalized
            if (setting == universalAgentSetting && jobPath == StandardJobPath)
              ignoreException(logger.asLazy.error) {  // Sometimes the connection is closed before JobScheduler can be notified about process termination ??? Then we get ReturnCode(1)
                assert(normalizedReturnCode == setting.returnCode(SIGKILL))
              }
            else
              normalizedReturnCode shouldEqual (if (jobPath == StandardJobPath) setting.returnCode(SIGKILL) else ReturnCode(1))   // Warum nicht auch SIGKILL ???
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

        if (setting eq universalAgentSetting) {
          "Kill script has been called" in {
            // Each filename in the directory is the argument list of the kill script call.
            val names = killScriptCallsDir.pathSet map { _.getFileName.toString }
            val expectedArg = "-kill-agent-task-id="
            for (name ← names) {
              assert(name contains expectedArg)
              assert(name.length > expectedArg.length)
            }
            assert(names.size == jobPaths.size)  // For each kill there is one file with unique AgentTaskId
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
      val run = startJob(WindowsJobPath)
      interceptSchedulerError(MessageCode("SCHEDULER-490")) {
        awaitSuccess(run.started)
        scheduler executeXml <kill_task job={WindowsJobPath.string} id={run.taskId.string} immediately="true" timeout="3"/>
      }
    }
  }

  private def addUnixTests(): Unit = {
    for (setting ← settings) {
      s"$setting" - {
        s"(preparation: run and kill tasks)" in {
          deleteAndWriteConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = setting.agentUriOption.toList))
          //controller.toleratingErrorCodes(Set("Z-REMOTE-101", "ERRNO-32", "SCHEDULER-202", "SCHEDULER-279", "SCHEDULER-280") map MessageCode) {
          controller.toleratingErrorLogged(_ ⇒ true) {
            val jobPaths = List(
              StandardJobPath, StandardMonitorJobPath,
              TrapJobPath, TrapMonitorJobPath,
              IgnoringJobPath, IgnoringMonitorJobPath,
              ApiJobPath)
            val runs = jobPaths map { startJob(_) }
            runs map { _.started } await 2*60.s
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
            results(jobPath).returnCode.normalized shouldEqual setting.returnCode(SIGTERM)
            assert(jobOverview(jobPath).state == JobState.stopped)
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

        for (jobPath ← List(TrapJobPath, TrapMonitorJobPath)) {
          s"$jobPath - With SIGTERM trapped, SIGTERM aborts process after signal was handled" in {
            results(jobPath).logString should (not include FinishedNormally and include(SigtermTrapped))
            results(jobPath).duration should be < UndisturbedDuration
            results(jobPath).endedInstant should be < killTime + MaxKillDuration + TrapDuration
            results(jobPath).returnCode shouldEqual ReturnCode(7)
            assert(jobOverview(jobPath).state == JobState.stopped)
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

        for (jobPath ← List(IgnoringJobPath, IgnoringMonitorJobPath)) {
          s"$jobPath - With SIGTERM ignored, timeout take effect" in {
            results(jobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
            results(jobPath).endedInstant should be > killTime + KillTimeout - 1.s
            results(jobPath).endedInstant should be < killTime + MaxKillDuration + KillTimeout
            results(jobPath).duration should be < UndisturbedDuration
            val normalizedReturnCode = results(jobPath).returnCode
            if (setting == universalAgentSetting && jobPath == IgnoringJobPath) {
              val expected = setting.returnCode(SIGKILL)
              ignoreException(logger.asLazy.error) {  // Sometimes the connection is closed before JobScheduler can be notified about process termination ??? Then we get ReturnCode(1)
                assert(normalizedReturnCode == expected)
              }
            } else
              // Why not this ??? results(jobPath).returnCode.normalized shouldEqual ReturnCode(SIGKILL)
              results(jobPath).returnCode.normalized shouldEqual (if (jobPath == IgnoringJobPath) setting.returnCode(SIGKILL) else ReturnCode(1))   // Warum nicht auch SIGKILL ???
            assert(jobOverview(jobPath).state == JobState.stopped)
            scheduler executeXml ModifyJobCommand(jobPath, cmd = Some(Unstop))
          }
        }

        for ((jobPath, expectedSpoolerProcessResult) ← List(StandardMonitorJobPath → false, TrapMonitorJobPath → false/*, IgnoringMonitorJobPath → true ???*/)) {
          s"$jobPath - spooler_process_after was called" in {
            results(jobPath).logString should include(TestMonitor.spoolerProcessAfterString(expectedSpoolerProcessResult))
          }
        }

        s"$setting - API job has been directly aborted" in {
          results(ApiJobPath).logString should (not include FinishedNormally and not include SigtermTrapped)
          results(ApiJobPath).endedInstant should be < killTime + 2.s
        }
      }
    }
  }

  "Agent has unregistered all tasks" in {
    requireNoTasksAreRegistered()
  }

  private def requireNoTasksAreRegistered(): Unit = {
    // C++ code does not await result of CloseTask. So we may wait a little.
    Exceptions.repeatUntilNoException(2.s, 50.ms) {
      val tasks = awaitSuccess(agentClient.task.tasks)
      assert(tasks.isEmpty)
    }
  }
}

private[js1163] object JS1163IT {
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
  private[js1163] val UndisturbedDuration = 40.s

  private val SigtermTrapped = "SIGTERM HANDLED"
  private val FinishedNormally = "FINISHED NORMALLY"

  private val logger = Logger(getClass)

  private trait Setting {
    def name: String
    def agentUriOption: Option[AgentAddress]
    def returnCode(signal: ProcessSignal): ReturnCode
    override final def toString = name
  }

  private object NoAgentSetting extends Setting {
    def name = "Without agent"
    def agentUriOption = None
    def returnCode(signal: ProcessSignal): ReturnCode =
      if (isWindows) {
        require(signal == SIGKILL)
        ReturnCode(99)  // C++ code terminates a Windows process with 99
      }
      else
        ReturnCode(signal)
  }

  private class UniversalAgentSetting(agentUri: () ⇒ AgentAddress) extends Setting {
    def name = "With Universal Agent"
    def agentUriOption = Some(agentUri())
    def returnCode(signal: ProcessSignal): ReturnCode =
      if (isWindows) {
        require(signal == SIGKILL)
        ReturnCode(1)  // Java's Process.destroyForcibly terminates with 1
      }
      else if (isSolaris)
        ReturnCode(signal.value)  // SIGKILL -> 9, SIGTERM -> 15
      else
        ReturnCode(signal)  // SIGKILL -> 137, SIGTERM -> 143
  }
}
