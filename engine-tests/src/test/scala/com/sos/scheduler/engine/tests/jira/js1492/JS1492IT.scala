package com.sos.scheduler.engine.tests.jira.js1492

import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.system.OperatingSystem.isWindows
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1492.JS1492IT._
import java.io.OutputStreamWriter
import java.net.Socket
import java.nio.charset.StandardCharsets.UTF_8
import java.time.Instant.now
import java.time.format.DateTimeFormatter
import java.time.{LocalDateTime, ZoneId}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1492 JobScheduler hangs if an order job has an end time in its current run time period and the task of the job needs more time.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1492IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))

  "JS-1492" in {
    val periodEnd =  now() + 3.s
    val expectedTaskEnd = now() + 5.s
    writeConfigurationFile(TestJobPath,
      <job>
        <script language="shell">{
          if (isWindows) "ping -n 6 127.0.0.1 >nul" else "sleep 5"
        }</script>
        <run_time>
          <period end={DateTimeFormatter.ofPattern("HH:mm:ss").format(LocalDateTime.ofInstant(periodEnd, instance[ZoneId]))}/>
        </run_time>
      </job>)
    val run = startOrder(JobChainPath("/test") orderKey "1")
    sleepUntil(periodEnd + 100.ms)
    checkTcp()
    assert(now() < expectedTaskEnd - 1.s)
    awaitSuccess(run.result)
    assert(now() > expectedTaskEnd)
    assert(now() < expectedTaskEnd + 3.s)
  }

  private def checkTcp(): Unit =
    autoClosing(new Socket("127.0.0.1", tcpPort)) { socket ⇒
      val writer = new OutputStreamWriter(socket.getOutputStream, UTF_8)
      writer.write("<show_state/>")
      writer.flush()
      assert(socket.getInputStream.read() == '<')
      logger.info("TCP works")
    }
}

private object JS1492IT {
  private val logger = Logger(getClass)
  private val TestJobPath = JobPath("/test")
}
