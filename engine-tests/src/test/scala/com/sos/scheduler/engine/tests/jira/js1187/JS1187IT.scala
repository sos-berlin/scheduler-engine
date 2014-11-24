package com.sos.scheduler.engine.tests.jira.js1187

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.system.Files.makeDirectory
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.log.{ErrorLogEvent, WarningLogEvent}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.kernel.job.TaskState
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.{job, runJobAndWaitForEnd, runJobFuture, task}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1187.JS1187IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Await

/**
 * JS-1187 Job waits until agent is available.
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
    def ignorable(e: ErrorLogEvent) =
      (e.message contains "spray.http.IllegalUriException") || e.codeOption() == Some(MessageCode("SCHEDULER-280"))
    controller.toleratingErrorLogEvent(ignorable) {  // Z-JAVA-105  Java exception spray.http.IllegalUriException: Invalid port 99999, method=CallObjectMethodA []
      runJobAndWaitForEnd(InvalidRemoteJobPath)
    }
    job(InvalidRemoteJobPath).isPermanentlyStopped shouldBe true
  }

  "(prepare process class)" in {
    scheduler executeXml <process_class name="agent" remote_scheduler={agentUri}/>
  }

  "With unreachable remote_scheduler, the waiting task can be killed" in {
    val warningFuture = controller.getEventBus.eventFuture[WarningLogEvent](_.codeOption == Some(MessageCode("SCHEDULER-488")))
    val (taskId, taskClosedFuture) = runJobFuture(UnreachableRemoteJobPath)
    Await.result(warningFuture, TestTimeout)
    requireIsWaitingForAgent(taskId)
    scheduler executeXml <kill_task job={UnreachableRemoteJobPath.string} id={taskId.string}/>
    Await.result(taskClosedFuture, TestTimeout)
  }

  "With unreachable remote_scheduler, task waits until agent is reachable" in {
    autoClosing(newExtraScheduler(agentHttpPort)) { agent ⇒
      val warningFuture = controller.getEventBus.eventFuture[WarningLogEvent](_.codeOption == Some(MessageCode("SCHEDULER-488")))
      val (taskId, taskClosedFuture) = runJobFuture(UnreachableRemoteJobPath)
      Await.result(warningFuture, TestTimeout)
      requireIsWaitingForAgent(taskId)
      agent.start()
      Await.result(agent.activatedFuture, TestTimeout)
      Await.result(taskClosedFuture, TestTimeout)
    }
  }

  private def requireIsWaitingForAgent(taskId: TaskId, expected: Boolean = true): Unit = {
    (scheduler.executeXml(<show_task id={taskId.string}/>).answer \ "task" \@ "waiting_for_remote_scheduler").toString.toBoolean shouldEqual expected
    task(taskId).state shouldEqual TaskState.waiting_for_process
  }

  private def newExtraScheduler(httpPort: Int) = {
    val logDir = controller.environment.logDirectory / s"agent-$httpPort"
    makeDirectory(logDir)
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${controller.environment.sosIniFile}",
      s"-ini=${controller.environment.iniFile}",
      s"-id=agent-$httpPort",
      s"-roles=agent",
      s"-log-dir=$logDir",
      s"-log-level=debug9",
      s"-log=${logDir / "scheduler.log"}",
      s"-java-classpath=${System.getProperty("java.class.path")}",
      s"-job-java-classpath=${System.getProperty("java.class.path")}",
      (controller.environment.configDirectory / "agent-scheduler.xml").getPath)
    new ExtraScheduler(args = args, httpPort = Some(httpPort))
  }
}

private object JS1187IT {
  private val InvalidRemoteJobPath = JobPath("/test-invalid-agent")
  private val UnreachableRemoteJobPath = JobPath("/test")
}
