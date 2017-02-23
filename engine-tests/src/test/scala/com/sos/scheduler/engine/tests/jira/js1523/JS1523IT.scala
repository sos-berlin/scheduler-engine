package com.sos.scheduler.engine.tests.jira.js1523

import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.job.ReturnCode
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.jobscheduler.http.client.heartbeat.{HeartbeatRequestor, HttpHeartbeatTiming}
import com.sos.jobscheduler.http.server.heartbeat.HeartbeatService
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.ImplicitTimeout
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1523.JS1523IT._
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
  * JS-1523 Agent with HTTP heartbeat.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1523IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private val timing = HttpHeartbeatTiming(period = 1.s, timeout = 2.s)
  private val delay = 2 * (timing.timeout + 2.s)

  protected override def onSchedulerActivated() = {
    super.onSchedulerActivated()
    writeConfigurationFile(ProcessClassPath("/test-agent"),
      <process_class>
        <remote_schedulers>
          <remote_scheduler
            remote_scheduler={agentUri.toString}
            http_heartbeat_period={timing.period.getSeconds.toString}
            http_heartbeat_timeout={timing.timeout.getSeconds.toString}/>
        </remote_schedulers>
      </process_class>)
  }

  "Server-side and client-side heartbeats keep the tasks alive" in {
    val serverSide = startJob(SleepJobPath)
    val clientSide = startJob(DelaySpoolerProcessJobPath, Map("delay" → delay.getSeconds.toString))
    awaitSuccess(serverSide.result).returnCode shouldEqual ReturnCode.Success
    awaitSuccess(clientSide.result).returnCode shouldEqual ReturnCode.Success
  }

  "With responds to server-side heartbeats being suppressed, task connection gets lost" in {
    try {
      HeartbeatService.staticSupressed = true
      testLostTask {
        runJob(SleepJobPath)
      }
    }
    finally HeartbeatService.staticSupressed = false
  }

  s"With client-side being suppressed, task connection gets lost" in {
    // While waiting between two spooler_process(), two client-side heartbeats are sent to the Agent, keeping the task alive
    implicit val implicitTimeout = ImplicitTimeout(delay + 10.s)
    val debug = instance[HeartbeatRequestor.Debug]
    try {
      debug.suppressed = true
      testLostTask {
        runJob(DelaySpoolerProcessJobPath, Map("delay" → delay.getSeconds.toString))
      }
    }
    finally debug.suppressed = false
  }

  "Task ends with error when client times out" in {
    val debug = instance[HeartbeatRequestor.Debug]
    try {
      debug.clientTimeout = Some(1.s)
      val t = now
      testLostTask {
        runJob(SleepJobPath)
      }
      val duration = now - t
      assert(duration >= debug.clientTimeout.get && duration < SleepJobDuration)
    }
    finally debug.clientTimeout = None
  }

  def testLostTask(body: ⇒ TaskResult): TaskResult = {
    var result: TaskResult = null
    autoClosing(controller.newEventPipe()) { events ⇒
      val expectedErrorCodes = Set(
        MessageCode("SCHEDULER-202"),
        MessageCode("SCHEDULER-280"),
        MessageCode("ERRNO-32"),
        MessageCode("ERRNO-131"),  // Solaris
        MessageCode("Z-REMOTE-101"),
        MessageCode("WINSOCK-10053"))
      controller.toleratingErrorCodes(expectedErrorCodes) {
        result = body
        result.returnCode shouldEqual ReturnCode.StandardFailure
      }
      events.queued[ErrorLogged] flatMap { _.event.codeOption } contains MessageCode("SCHEDULER-202")
      // How to check the process has actually being killed ??? EventBus!
    }
    result
  }
}

private object JS1523IT {
  private val DelaySpoolerProcessJobPath = JobPath("/test-delay_spooler_process")
  private val SleepJobPath = JobPath("/test-sleep")
  private val SleepJobDuration = 10.s
}
