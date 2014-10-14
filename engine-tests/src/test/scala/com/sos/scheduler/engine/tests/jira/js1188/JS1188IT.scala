package com.sos.scheduler.engine.tests.jira.js1188

import com.google.common.io.Closer
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.system.Files.makeDirectory
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.{alternateTcpPortRange, findRandomFreeTcpPort}
import com.sos.scheduler.engine.data.filebased.FileBasedReplacedEvent
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent, TaskId}
import com.sos.scheduler.engine.data.log.WarningLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.TaskState
import com.sos.scheduler.engine.kernel.processclass.ProcessClass
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.{awaitResult, awaitResults, executionContext, processClass, runJobAndWaitForEnd, runJobFuture, task}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.jira.js1188.JS1188IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.Vector.fill
import scala.collection.immutable
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1188IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = FreeTcpPortFinder.findRandomFreeTcpPort()
  private lazy val agentRefs = List.fill(n) { new AgentRef().registerCloseable }
  private var waitingTaskClosedFuture: Future[TaskClosedEvent] = null
  private var waitingStopwatch: Stopwatch = null

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"))

  "reduceRepeating" in {
    val x = -9
    assertResult(Vector(x, 1, x, 2, x, 3, 4, x)) {
      reduceRepeating(x)(Vector(x, x, 1, x, 2, x, 3, 4, x, x, x))
    }
  }

  "ignoreExtraEntries" in {
    val x = -9
    val expected = Vector(1, 2, 3, x, 4, 5, 6, x)
    assertResult(expected) {
      ignoreExtraEntries(expected, x)(Vector(x, 1, 2, 3, x, x, 4, 5, 6, x))
    }
  }

  "(prepare process class)" in {
    scheduler executeXml processClassXml("agents", agentRefs)
  }

  s"With unreachable agents, task waits 2 times ${ProcessClass.InaccessibleAgentDelay.pretty} because no agent is reachable" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      val (waitingTaskId, taskClosedFuture) = runJobFuture(TestJobPath)
      waitingTaskClosedFuture = taskClosedFuture
      waitingStopwatch = new Stopwatch
      sleep(1.s)
      requireTaskIsWaitingForAgent(waitingTaskId)
      sleep((2.5 * ProcessClass.InaccessibleAgentDelay.getMillis).toLong)
      requireTaskIsWaitingForAgent(waitingTaskId)
      val expectedWarnings = fill(3)(fill(n)(InaccessibleAgentMessageCode) :+ WaitingForAgentMessageCode).flatten map Some.apply
      assertResult(expectedWarnings) {
        val codeOptions = eventPipe.queued[WarningLogEvent].toVector map { _.codeOption }
        ignoreExtraWaitingForAgentMessageCode(expectedWarnings)(codeOptions)
      }
      waitingStopwatch.duration should be > 2*ProcessClass.InaccessibleAgentDelay  // Process class is still waiting the 3rd time
    }
  }

  "After starting 2 agents and after process class has waited the third cycle, the waiting task can be finished" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      startAndWaitForAgents(agentRefs(1), agentRefs(3)) // Start 2 out of n agents
      awaitResult(waitingTaskClosedFuture) // Waiting task has finally finished
      waitingStopwatch.duration should be > 3 * ProcessClass.InaccessibleAgentDelay
      // Agent 0 is still unreachable
      assertResult(List(Some(InaccessibleAgentMessageCode))) {
        eventPipe.queued[WarningLogEvent] map { _.codeOption } filter { _ != Some(WaitingForAgentMessageCode) }
      }
    }
  }

  s"And more tasks start immediately before reaching next probe time after ${ProcessClass.InaccessibleAgentDelay.pretty}" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      val stopwatch = new Stopwatch
      for (_ ← 1 to 2*n + 1) runJobAndWaitForEnd(TestJobPath)
      stopwatch.duration should be < TestTimeout
      eventPipe.queued[WarningLogEvent] map { _.codeOption } shouldEqual List(Some(InaccessibleAgentMessageCode))  // Agent 2 is still unreachable
    }
  }

  "Replacing configuration file .process_class.xml" in {
    assertResult(List(Agent(0, "http://xxx"), Agent(1, "http://yyy"))) {
      processClass(AProcessClassPath).agents
    }
    controller.getEventBus.awaitingKeyedEvent[FileBasedReplacedEvent](AProcessClassPath) {
      Files.copy(testEnvironment.fileFromPath(BProcessClassPath), testEnvironment.fileFromPath(AProcessClassPath))
      instance[FolderSubsystem].updateFolders()
    }
    assertResult(List(Agent(0, "http://aaa"), Agent(1, "http://bbb"))) {
      processClass(AProcessClassPath).agents
    }
  }

  private def startAndWaitForAgents(agentRefs: AgentRef*)(implicit closer: Closer): Unit = {
    awaitResults(
      for (a ← agentRefs) yield {
        newAgent(a).registerCloseable.start()
      })
  }

  private def requireTaskIsWaitingForAgent(taskId: TaskId, expected: Boolean = true): Unit = {
    (scheduler.executeXml(<show_task id={taskId.string}/>).answer \ "task" \@ "waiting_for_remote_scheduler").toBoolean shouldEqual expected
    task(taskId).state shouldEqual TaskState.waiting_for_process
  }

  private def newAgent(agentRef: AgentRef) = {
    //agentRef.reservingSocket.close()
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
    new ExtraScheduler(args = args, env = Map(), httpPort = Some(agentRef.port))
  }
}

private object JS1188IT {
  private val n = 4
  private val TestJobPath = JobPath("/test")
  private val AProcessClassPath = ProcessClassPath("/test-a")
  private val BProcessClassPath = ProcessClassPath("/test-b")
  private val InaccessibleAgentMessageCode = MessageCode("SCHEDULER-488")
  private val WaitingForAgentMessageCode = MessageCode("SCHEDULER-489")

  private class AgentRef extends AutoCloseable {
    val port = findRandomFreeTcpPort(alternateTcpPortRange)
    //val reservingSocket = new ServerSocket(port, 1)
    def uri = s"http://127.0.0.1:$port"
    def close(): Unit = ()//reservingSocket.close()
  }

  private def processClassXml(name: String, agentRefs: Seq[AgentRef]) =
    <process_class name={name}>
      <remote_schedulers>{
        agentRefs map { o ⇒ <remote_scheduler remote_scheduler={o.uri}/> }
      }</remote_schedulers>
    </process_class>

  private def reduceRepeating[A](value: A)(seq: Vector[A]): immutable.IndexedSeq[A] =
    0 +: ((1 until seq.size) filterNot { i ⇒ seq(i - 1) == value && seq(i) == value }) map seq

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
