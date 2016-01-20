package com.sos.scheduler.engine.tests.jira.js1187

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.log.{ErrorLogEvent, WarningLogEvent}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.job.TaskState
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.{awaitSuccess, job, startJob, task}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1187.JS1187IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1187 Job waits until agent is available.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1187IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val Seq(tcpPort, agentHttpPort) = findRandomFreeTcpPorts(2)
  private lazy val agentUri = s"http://127.0.0.1:$agentHttpPort"

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"))

  "With invalid remote_scheduler address, task does not start" in {
    def ignorable(e: ErrorLogEvent) = (e.message contains "spray.http.IllegalUriException") || e.codeOption() == Some(MessageCode("SCHEDULER-280"))
    controller.toleratingErrorLogEvent(ignorable) {  // Z-JAVA-105  Java exception spray.http.IllegalUriException: Invalid port 99999, method=CallObjectMethodA []
      awaitSuccess(startJob(InvalidRemoteJobPath).closed)
    }
    job(InvalidRemoteJobPath).isPermanentlyStopped shouldBe true
  }

  "(prepare process class)" in {
    scheduler executeXml <process_class name="agent" remote_scheduler={agentUri}/>
  }

  "With unreachable remote_scheduler, the waiting task can be killed" in {
    val warningFuture = eventBus.eventFuture[WarningLogEvent](_.codeOption == Some(MessageCode("SCHEDULER-488")))
    val taskRun = startJob(UnreachableRemoteJobPath)
    awaitResult(warningFuture, TestTimeout)
    requireIsWaitingForAgent(taskRun.taskId)
    scheduler executeXml <kill_task job={UnreachableRemoteJobPath.string} id={taskRun.taskId.string}/>
    awaitResult(taskRun.closed, TestTimeout)
  }

  "With unreachable remote_scheduler, task waits until agent is reachable" in {
    autoClosing(Agent.forTest(agentHttpPort)) { agent ⇒
      val warningFuture = eventBus.eventFuture[WarningLogEvent](_.codeOption == Some(MessageCode("SCHEDULER-488")))
      val taskRun = startJob(UnreachableRemoteJobPath)
      awaitResult(warningFuture, TestTimeout)
      requireIsWaitingForAgent(taskRun.taskId)
      awaitResult(agent.start(), TestTimeout)
      awaitResult(taskRun.result, TestTimeout)
    }
  }

  private def requireIsWaitingForAgent(taskId: TaskId, expected: Boolean = true): Unit = {
    (scheduler.executeXml(<show_task id={taskId.string}/>).answer \ "task" \@ "waiting_for_remote_scheduler").toString.toBoolean shouldEqual expected
    task(taskId).state shouldEqual TaskState.waiting_for_process
  }
}

private object JS1187IT {
  private val InvalidRemoteJobPath = JobPath("/test-invalid-agent")
  private val UnreachableRemoteJobPath = JobPath("/test")
}
