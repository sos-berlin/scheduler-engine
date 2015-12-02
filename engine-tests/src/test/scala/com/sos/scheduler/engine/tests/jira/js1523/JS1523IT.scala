package com.sos.scheduler.engine.tests.jira.js1523

import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode}
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestor
import com.sos.scheduler.engine.http.server.heartbeat.HeartbeatService
import com.sos.scheduler.engine.test.ImplicitTimeout
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
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

  private val heartbeatTimeout = 2.s
  private val delay = 2 * (heartbeatTimeout + 2.s)

  protected override def onSchedulerActivated() = {
    super.onSchedulerActivated()
    writeConfigurationFile(ProcessClassPath("/test-agent"),
      <process_class>
        <remote_schedulers>
          <remote_scheduler remote_scheduler={agentUri.toString} http_heartbeat_timeout={heartbeatTimeout.getSeconds.toString} http_heartbeat_period="1"/>
        </remote_schedulers>
      </process_class>)
  }

  "Server-side and client-side heartbeats keep the tasks alive" in {
    val serverSide = runJobFuture(JobPath("/test-sleep"))
    val clientSide = runJobFuture(JobPath("/test-delay_spooler_process"), Map("delay" → delay.getSeconds.toString))
    awaitSuccess(serverSide.result).returnCode shouldEqual ReturnCode.Success
    awaitSuccess(clientSide.result).returnCode shouldEqual ReturnCode.Success
  }

  "With responds to server-side heartbeats being suppressed, task connection gets lost" in {
    try {
      agent.injector.instance[HeartbeatService.Debug].suppressed = true
      testLostTask {
        runJobAndWaitForEnd(JobPath("/test-sleep"))
      }
    }
    finally agent.injector.instance[HeartbeatService.Debug].suppressed = false
  }

  s"With client-side being suppressed, task connection gets lost" in {
    // While waiting between two spooler_process(), two client-side heartbeats are sent to the Agent, keeping the task alive
    implicit val implicitTimeout = ImplicitTimeout(delay + 10.s)
    try {
      instance[HeartbeatRequestor.Debug].suppressed = true
      testLostTask {
        runJobAndWaitForEnd(JobPath("/test-delay_spooler_process"), Map("delay" → delay.getSeconds.toString))
      }
    }
    finally instance[HeartbeatRequestor.Debug].suppressed = false
  }

  def testLostTask(body: ⇒ TaskResult): Unit = {
    autoClosing(controller.newEventPipe()) { events ⇒
      controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-202"), MessageCode("SCHEDULER-280"), MessageCode("WINSOCK-10053"), MessageCode("ERRNO-32"), MessageCode("Z-REMOTE-101"))) {
        body.returnCode shouldEqual ReturnCode.StandardFailure
      }
      events.queued[ErrorLogEvent] flatMap { _.codeOption } contains MessageCode("SCHEDULER-202")
      // How to check the process has actually being killed ??? EventBus!
    }
  }
}
