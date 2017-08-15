package com.sos.scheduler.engine.tests.jira.js1188

import com.google.common.io.Closer
import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.job.{JobPath, JobState, TaskId, TaskObstacle, TaskState}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.log.{ErrorLogged, WarningLogged}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderWaitingInTask}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.{OrderCommand, ProcessClassConfiguration}
import com.sos.scheduler.engine.kernel.processclass.common.FailableSelector
import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1188.JS1188IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.Vector.fill
import scala.collection.mutable
import scala.concurrent.Promise

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1188IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val agentTcpPorts = findRandomFreeTcpPorts(n)
  private lazy val agentRefs = (0 until n).toList zip agentTcpPorts map { case (i, port) ⇒ AgentRef(('A' + i).toChar.toString, port) } ensuring { _.size == n }
  private lazy val runningAgents = mutable.Map[AgentRef, Agent]()
  private var waitingTaskRun: TaskRun = null
  private val orderFinished = Promise[Unit]()
  private var waitingStopwatch: Stopwatch = null

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    cppSettings = Map(CppSettingName.agentConnectRetryDelay → AgentConnectRetryDelay.getSeconds.toString))

  "ignoreExtraEntries" in {
    val x = -9
    val expected = Vector(1, 2, 3, x, 4, 5, 6, x)
    assertResult(expected) {
      ignoreExtraEntries(expected, x)(Vector(x, 1, 2, 3, x, x, 4, 5, 6, x))
    }
  }

  "(prepare process class)" in {
    writeConfigurationFile(AgentsProcessClassPath, ProcessClassConfiguration(agentUris = agentRefs map { _.uri }))
  }

  "Job-API process_class.remote_scheduler can be changed only with (old) non-HTTP agents" in {
    runJob(JobPath("/test-api"))
    jobOverview(JobPath("/test-api")).state should not equal JobState.stopped
  }

  "With unreachable agents, task waits 2 times agentConnectRetryDelay because no agent is reachable" in {
    withEventPipe { eventPipe ⇒
      waitingTaskRun = startJob(AgentsJobPath)
      waitingStopwatch = new Stopwatch
      sleep(1.s)
      requireTaskIsWaitingForAgent(waitingTaskRun.taskId)
      sleep((2.5 * AgentConnectRetryDelay.toMillis).toLong)
      requireTaskIsWaitingForAgent(waitingTaskRun.taskId)
      val expectedWarnings = fill(3)(fill(n)(InaccessibleAgentMessageCode) :+ WaitingForAgentMessageCode).flatten map Some.apply
      assertResult(expectedWarnings) {
        val codeOptions = eventPipe.queued[WarningLogged].toVector map { _.event.codeOption }
        ignoreExtraWaitingForAgentMessageCode(expectedWarnings)(codeOptions)
      }
      waitingStopwatch.duration should be > 2*AgentConnectRetryDelay  // Process class is still waiting the 3rd time
      waitingStopwatch.duration should be < 3*AgentConnectRetryDelay
    }
  }

  "Starting an order for a not-running agent" in {
    eventBus.on[OrderFinished] {
      case KeyedEvent(TestOrderKey, OrderFinished(NodeId("END"))) ⇒
        orderFinished.success(())
    }
    eventBus.awaiting[OrderWaitingInTask.type](TestOrderKey) {
      startOrder(OrderCommand(TestOrderKey))
    }
  }

  "After starting 2 agents and after process class has waited the third cycle, the waiting task can be finished" in {
    withEventPipe { eventPipe ⇒
      startAndWaitForAgents(agentRefs(1), agentRefs(3)) // Start 2 out of n agents
      awaitSuccess(waitingTaskRun.closed) // Waiting task has finally finished
      assert(waitingTaskRun.logString contains agentRefs(1).testOutput)
      waitingStopwatch.duration should be > 3*AgentConnectRetryDelay
      //Not on a busy computer: waitingStopwatch.duration should be < 4*AgentConnectRetryDelay
      // Agent 0 is still unreachable
      //(Following check is not reliable. Number of messages "SCHEDULER-489" depends on run-time characteristic. */
      //assertResult(List(Some(InaccessibleAgentMessageCode))) {
      //  eventPipe.queued[WarningLogged] map { _.codeOption } filter { _ != Some(WaitingForAgentMessageCode) }
      //}
    }
  }

  "The order can be carried out now" in {
    orderFinished.future await TestTimeout
  }

  "After agentConnectRetryDelay, more tasks start immediately before reaching next probe time, FixedPriority" in {
    withEventPipe { eventPipe ⇒
      val stopwatch = new Stopwatch
      val results = for (_ ← 1 to 2*n + 1) yield runJob(AgentsJobPath)
      val taskAgentNames = results flatMap taskResultToAgentName
      assert(taskAgentNames == List.fill(2*n + 1) { agentRefs(1).name })  // B B B B B B B B B
      stopwatch.duration should be < TestTimeout
      //Not reliable (all tasks can start on first Agent): eventPipe.queued[WarningLogged] map { _.codeOption } shouldEqual List(Some(InaccessibleAgentMessageCode))  // Agent 2 is still unreachable
    }
  }

  "RoundRobin" in {
    writeConfigurationFile(AgentsProcessClassPath, ProcessClassConfiguration(agentUris = agentRefs map { _.uri }, select = Some("next")))
    withEventPipe { eventPipe ⇒
      val stopwatch = new Stopwatch
      val results = for (_ ← 1 to 2*n + 1) yield runJob(AgentsJobPath)
      val taskAgentNames = results flatMap taskResultToAgentName
      def alternatingAgentNames = (Iterator continually { List(agentRefs(1).name, agentRefs(3).name) }).flatten
      assert(taskAgentNames == alternatingAgentNames.slice(0, 2 * n + 1).toList ||  // B D B D B D B D B or
             taskAgentNames == alternatingAgentNames.slice(1, 2 * n + 2).toList)    // D B D B D B D B D
      stopwatch.duration should be < TestTimeout
      eventPipe.queued[WarningLogged] map { _.event.codeOption } should contain (Some(InaccessibleAgentMessageCode))  // First Agent is still unreachable
    }
  }

  "Replacing configuration file .process_class.xml" in {
    withEventPipe { eventPipe ⇒
      assertResult(List(AgentAddress("http://127.0.0.254:1"), AgentAddress("http://127.0.0.253:1"))) {
        processClass(ReplaceProcessClassPath).agents map { _.address }
      }
      val taskRun = startJob(ReplaceTestJobPath)
      eventPipe.nextAny[WarningLogged].event.codeOption shouldEqual Some(InaccessibleAgentMessageCode)
      writeConfigurationFile(ReplaceProcessClassPath, ProcessClassConfiguration(agentUris = List(agentRefs(1).uri)))
      assertResult(List(agentRefs(1).uri)) {
        processClass(ReplaceProcessClassPath).agents map { _.address }
      }
      // Job should run now with new process class configuration denoting an accessible agent
      awaitSuccess(taskRun.result)
    }
  }

  "Replacing configuration file .process_class.xml, removing remote_schedulers" in {
    withEventPipe { eventPipe ⇒
      assertResult(List(AgentAddress(s"${agentRefs(1).uri}"))) {
        processClass(ReplaceProcessClassPath).agents map { _.address }
      }
      for (a ← runningAgents.values) {
        a.close()
      }
      val taskRun = startJob(ReplaceTestJobPath)
      eventPipe.nextAny[WarningLogged].event.codeOption shouldEqual Some(InaccessibleAgentMessageCode)
      def expectedErrorLogged(e: ErrorLogged) =
        e.codeOption == Some(MessageCode("SCHEDULER-280")) ||
        e.codeOption == Some(MessageCode("Z-JAVA-105")) && (e.message contains classOf[FailableSelector.CancelledException].getName)
      controller.toleratingErrorLogged(expectedErrorLogged) {
        writeConfigurationFile(ReplaceProcessClassPath, ProcessClassConfiguration())
        assertResult(Nil) {
          processClass(ReplaceProcessClassPath).agents map { _.address }
        }
        awaitSuccess(taskRun.closed)
      }
      // Job should run fail with SCHEDULER-280 because the new process class has no longer remote scheduler
      assert(eventPipe.queued[ErrorLogged] exists { _.event.codeOption == Some(MessageCode("SCHEDULER-280")) })
      assert(jobOverview(ReplaceTestJobPath).state == JobState.stopped)
    }
  }

  private def startAndWaitForAgents(agentRefs: AgentRef*)(implicit closer: Closer): Unit = {
    awaitResults(
      for (a ← agentRefs) yield {
        runningAgents(a) = newAgent(a).closeWithCloser
        runningAgents(a).start()
      })
  }

  private def requireTaskIsWaitingForAgent(taskId: TaskId, expected: Boolean = true): Unit = {
    (scheduler.executeXml(<show_task id={taskId.string}/>).answer \ "task" \@ "waiting_for_remote_scheduler").toBoolean shouldEqual expected
    taskOverview(taskId).state shouldEqual TaskState.waiting_for_process
    assert(taskOverview(taskId).obstacles contains TaskObstacle.WaitingForAgent)
  }

  private def newAgent(agentRef: AgentRef) =
    new Agent(AgentConfiguration.forTest(httpPort = agentRef.port)
      .copy(environment = Map("TEST_AGENT_NAME" → s"${agentRef.name}")))

  private def taskResultToAgentName(taskResult: TaskResult): Option[String] =
    for (m ← AgentNameRegex.findFirstMatchIn(taskResult.logString)) yield m.group(1)
}

private object JS1188IT {
  private val TestOrderKey = JobChainPath("/test") orderKey "TEST"
  private val TestJobPath = JobPath("/test")
  private val AgentConnectRetryDelay = 15.s
  private val n = 4
  private val AgentsProcessClassPath = ProcessClassPath("/agents")
  private val AgentsJobPath = JobPath("/test")
  private val ReplaceProcessClassPath = ProcessClassPath("/test-replace")
  private val ReplaceTestJobPath = JobPath("/test-a")
  private val InaccessibleAgentMessageCode = MessageCode("SCHEDULER-488")
  private val WaitingForAgentMessageCode = MessageCode("SCHEDULER-489")
  private val AgentNameRegex = "TEST_AGENT_NAME=/(.*)/".r

  private case class AgentRef(name: String, port: Int) {
    def uri = AgentAddress(s"http://127.0.0.1:$port")
    def testOutput = s"TEST_AGENT_NAME=/$name/"
  }

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
