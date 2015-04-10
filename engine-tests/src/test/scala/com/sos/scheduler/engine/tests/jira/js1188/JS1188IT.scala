package com.sos.scheduler.engine.tests.jira.js1188

import com.google.common.io.Closer
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.system.Files.makeDirectory
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.filebased.FileBasedReplacedEvent
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent, TaskId}
import com.sos.scheduler.engine.data.log.{ErrorLogEvent, WarningLogEvent}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.{JobState, TaskState}
import com.sos.scheduler.engine.kernel.processclass.common.FailableSelector
import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.{awaitResults, awaitSuccess, executionContext, job, processClass, runJobAndWaitForEnd, runJobFuture, task}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1188.JS1188IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.Vector.fill
import scala.collection.mutable
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1188IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort :: agentTcpPorts = findRandomFreeTcpPorts(1 + n)
  private lazy val agentRefs = agentTcpPorts map AgentRef ensuring { _.size == n }
  private lazy val runningAgents = mutable.Map[AgentRef, ExtraScheduler]()
  private var waitingTaskClosedFuture: Future[TaskClosedEvent] = null
  private var waitingStopwatch: Stopwatch = null

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"),
    cppSettings = Map(CppSettingName.agentConnectRetryDelay → AgentConnectRetryDelay.getStandardSeconds.toString))

  "ignoreExtraEntries" in {
    val x = -9
    val expected = Vector(1, 2, 3, x, 4, 5, 6, x)
    assertResult(expected) {
      ignoreExtraEntries(expected, x)(Vector(x, 1, 2, 3, x, x, 4, 5, 6, x))
    }
  }

  "(prepare process class)" in {
    scheduler executeXml processClassXml(AgentsProcessClassPath.name, agentRefs)
  }

  "Job-API process_class.remote_scheduler can be changed only with (old) non-HTTP agents" in {
    runJobAndWaitForEnd(JobPath("/test-api"))
    job(JobPath("/test-api")).state should not equal JobState.stopped
  }

  "With unreachable agents, task waits 2 times agentConnectRetryDelay because no agent is reachable" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      val (waitingTaskId, taskClosedFuture) = runJobFuture(AgentsJobPath)
      waitingTaskClosedFuture = taskClosedFuture
      waitingStopwatch = new Stopwatch
      sleep(1.s)
      requireTaskIsWaitingForAgent(waitingTaskId)
      sleep((2.5 * AgentConnectRetryDelay.getMillis).toLong)
      requireTaskIsWaitingForAgent(waitingTaskId)
      val expectedWarnings = fill(3)(fill(n)(InaccessibleAgentMessageCode) :+ WaitingForAgentMessageCode).flatten map Some.apply
      assertResult(expectedWarnings) {
        val codeOptions = eventPipe.queued[WarningLogEvent].toVector map { _.codeOption }
        ignoreExtraWaitingForAgentMessageCode(expectedWarnings)(codeOptions)
      }
      waitingStopwatch.duration should be > 2*AgentConnectRetryDelay  // Process class is still waiting the 3rd time
      waitingStopwatch.duration should be < 3*AgentConnectRetryDelay
    }
  }

  "After starting 2 agents and after process class has waited the third cycle, the waiting task can be finished" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      startAndWaitForAgents(agentRefs(1), agentRefs(3)) // Start 2 out of n agents
      awaitSuccess(waitingTaskClosedFuture) // Waiting task has finally finished
      waitingStopwatch.duration should be > 3*AgentConnectRetryDelay
      //Not on a busy computer: waitingStopwatch.duration should be < 4*AgentConnectRetryDelay
      // Agent 0 is still unreachable
      //(Following check is not reliable. Number of messages "SCHEDULER-489" depends on run-time characteristic. */
      //assertResult(List(Some(InaccessibleAgentMessageCode))) {
      //  eventPipe.queued[WarningLogEvent] map { _.codeOption } filter { _ != Some(WaitingForAgentMessageCode) }
      //}
    }
  }

  "After agentConnectRetryDelay, more tasks start immediately before reaching next probe time" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      val stopwatch = new Stopwatch
      for (_ ← 1 to 2*n + 1) runJobAndWaitForEnd(AgentsJobPath)
      stopwatch.duration should be < TestTimeout
      //Not reliable (all tasks can start on first Agent): eventPipe.queued[WarningLogEvent] map { _.codeOption } shouldEqual List(Some(InaccessibleAgentMessageCode))  // Agent 2 is still unreachable
    }
  }

  "Replacing configuration file .process_class.xml" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      assertResult(List("http://127.0.0.254:1", "http://127.0.0.253:1")) {
        processClass(ReplaceProcessClassPath).agents map { _.address }
      }
      val (_, jobFuture) = runJobFuture(ReplaceTestJobPath)
      eventPipe.nextAny[WarningLogEvent].codeOption shouldEqual Some(InaccessibleAgentMessageCode)
      controller.eventBus.awaitingKeyedEvent[FileBasedReplacedEvent](ReplaceProcessClassPath) {
        testEnvironment.fileFromPath(ReplaceProcessClassPath).xml = processClassXml("test-replace", List(agentRefs(1)))
        instance[FolderSubsystem].updateFolders()
      }
      assertResult(List(agentRefs(1).uri)) {
        processClass(ReplaceProcessClassPath).agents map { _.address }
      }
      // Job should run now with new process class configuration denoting an accessible agent
      awaitSuccess(jobFuture)
    }
  }

  "Replacing configuration file .process_class.xml, removing remote_schedulers" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      assertResult(List(agentRefs(1).uri)) {
        processClass(ReplaceProcessClassPath).agents map { _.address }
      }
      for (a ← runningAgents.values) {
        a.close()
      }
      val (_, jobFuture) = runJobFuture(ReplaceTestJobPath)
      eventPipe.nextAny[WarningLogEvent].codeOption shouldEqual Some(InaccessibleAgentMessageCode)
      def expectedErrorLogEvent(e: ErrorLogEvent) =
        e.codeOption == Some(MessageCode("SCHEDULER-280")) ||
        e.codeOption == Some(MessageCode("Z-JAVA-105")) && (e.message contains classOf[FailableSelector.CancelledException].getName)
      controller.toleratingErrorLogEvent(expectedErrorLogEvent) {
        controller.eventBus.awaitingKeyedEvent[FileBasedReplacedEvent](ReplaceProcessClassPath) {
          testEnvironment.fileFromPath(ReplaceProcessClassPath).xml = processClassXml("test-replace", Nil)
          instance[FolderSubsystem].updateFolders()
        }
        assertResult(Nil) {
          processClass(ReplaceProcessClassPath).agents map { _.address }
        }
        awaitSuccess(jobFuture)
      }
      // Job should run fail with SCHEDULER-280 because the new process class has no longer remote scheduler
      assert(eventPipe.queued[ErrorLogEvent] exists { _.codeOption == Some(MessageCode("SCHEDULER-280")) })
      assert(job(ReplaceTestJobPath).state == JobState.stopped)
    }
  }

  private def startAndWaitForAgents(agentRefs: AgentRef*)(implicit closer: Closer): Unit = {
    awaitResults(
      for (a ← agentRefs) yield {
        runningAgents(a) = newAgent(a).registerCloseable
        runningAgents(a).start()
      })
  }

  private def requireTaskIsWaitingForAgent(taskId: TaskId, expected: Boolean = true): Unit = {
    (scheduler.executeXml(<show_task id={taskId.string}/>).answer \ "task" \@ "waiting_for_remote_scheduler").toBoolean shouldEqual expected
    task(taskId).state shouldEqual TaskState.waiting_for_process
  }

  private def newAgent(agentRef: AgentRef) = {
    val logDir = controller.environment.logDirectory / s"agent-${agentRef.port}"
    makeDirectory(logDir)
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${controller.environment.sosIniFile}",
      s"-ini=${controller.environment.iniFile}",
      s"-id=agent-${agentRef.port}",
      s"-roles=agent",
      s"-log-dir=$logDir",
      s"-log-level=debug9",
      s"-log=${logDir / "scheduler.log"}",
      s"-java-classpath=${System.getProperty("java.class.path")}",
      s"-job-java-classpath=${System.getProperty("java.class.path")}",
      (controller.environment.configDirectory / "agent-scheduler.xml").getPath)
    new ExtraScheduler(args = args, httpPort = Some(agentRef.port))
  }
}

private object JS1188IT {
  private val AgentConnectRetryDelay = 15.s
  private val n = 4
  private val AgentsProcessClassPath = ProcessClassPath("/agents")
  private val AgentsJobPath = JobPath("/test")
  private val ReplaceProcessClassPath = ProcessClassPath("/test-replace")
  private val ReplaceTestJobPath = JobPath("/test-a")
  private val InaccessibleAgentMessageCode = MessageCode("SCHEDULER-488")
  private val WaitingForAgentMessageCode = MessageCode("SCHEDULER-489")

  private case class AgentRef(port: Int) {
    def uri = s"http://127.0.0.1:$port"
  }

  private def processClassXml(name: String, agentRefs: Seq[AgentRef]) =
    <process_class name={name}>
      <remote_schedulers>{
        agentRefs map { o ⇒ <remote_scheduler remote_scheduler={o.uri}/> }
      }</remote_schedulers>
    </process_class>

  private def ignoreExtraWaitingForAgentMessageCode(expected: TraversableOnce[Option[MessageCode]])(seq: TraversableOnce[Option[MessageCode]]) =
    ignoreExtraEntries[Option[MessageCode]](expected, ignore = Some(WaitingForAgentMessageCode))(seq)

  private def ignoreExtraEntries[A](expected: TraversableOnce[A], ignore: A)(seq: TraversableOnce[A]): Vector[A] = {
    val i = seq.toIterator.buffered
    for (e ← expected.toVector) yield {
      while (i.head != e && i.head == ignore) i.next()
      i.next()
    }
  }
}
