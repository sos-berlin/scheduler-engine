package com.sos.scheduler.engine.tests.scheduler.job.stdout

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.job.stdout.StdoutIT._
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * Tests the periodical logging of a job's stdout and stderr output - without and with Java and C++ Agent.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class StdoutIT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"))

  private lazy val processClasses = Map(
    "Local" → <process_class name="test" replace="true"/>,
    "Universal Agent" → <process_class name="test" replace="true" remote_scheduler={agentUri}/>)

  for ((testName, processClass) ← processClasses) {
    s"stdout and stderr of shell, monitored shell and job jobs - $testName" in {
      scheduler executeXml processClass
      withEventPipe { eventPipe ⇒
        val shellRun = runJobFuture(JobPath("/test-shell"))
        val monitoredShellRun = runJobFuture(JobPath("/test-shell-monitor"))
        val javaRun = runJobFuture(JobPath("/test-java"))
        val firstPollAfter = now() + 14.s + StdoutPollingInterval

        awaitSuccess(shellRun.started)
        sleep(firstPollAfter - now())
        eventPipe.queued[InfoLogEvent] exists { _.message contains "SIMPLE-SHELL-STDOUT-1" }
        eventPipe.queued[InfoLogEvent] exists { _.message contains "SIMPLE-SHELL-STDERR-1" }

        awaitSuccess(monitoredShellRun.started)
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestMonitor.StartStdoutMessage }
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestMonitor.StartStderrMessage }
        eventPipe.queued[InfoLogEvent] exists { _.message contains "MONITORED-SHELL-STDOUT-1" }
        eventPipe.queued[InfoLogEvent] exists { _.message contains "MONITORED-SHELL-STDERR-1" }

        awaitSuccess(javaRun.started)
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestJob.StartStdoutMessage }
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestJob.StartStderrMessage }

        awaitSuccess(shellRun.closed)
        eventPipe.queued[InfoLogEvent] exists { _.message contains "SIMPLE-SHELL-STDOUT-2" }
        eventPipe.queued[InfoLogEvent] exists { _.message contains "SIMPLE-SHELL-STDERR-2" }

        awaitSuccess(monitoredShellRun.closed)
        eventPipe.queued[InfoLogEvent] exists { _.message contains "MONITORED-SHELL-STDOUT-2" }
        eventPipe.queued[InfoLogEvent] exists { _.message contains "MONITORED-SHELL-STDERR-2" }
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestMonitor.AfterStdoutMessage }
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestMonitor.AfterStderrMessage }

        awaitSuccess(javaRun.closed)
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestJob.StartStdoutMessage }
        eventPipe.queued[InfoLogEvent] exists { _.message contains TestJob.StartStderrMessage }
      }
    }
  }
}

private object StdoutIT {
  private val StdoutPollingInterval = 10.s
}
