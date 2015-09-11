package com.sos.scheduler.engine.tests.jira.js1492

import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.time.TimeZones
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1492.JS1492IT._
import java.io.OutputStreamWriter
import java.net.Socket
import java.nio.charset.StandardCharsets.UTF_8
import java.time.Instant.now
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
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
    writeConfigurationFile(TestJobPath,
      <job>
        <script language="shell">{
          if (isWindows) "ping -n 6 127.0.0.1 >nul" else "sleep 5"
        }</script>
        <run_time>
          <period end={DateTimeFormatter.ofPattern("HH:mm:ss").format(LocalDateTime.ofInstant(periodEnd, TimeZones.SchedulerLocalZoneId))}/>
        </run_time>
      </job>)
    val run = runJobFuture(TestJobPath)
    sleepUntil(periodEnd + 100.ms)
    val socket = new Socket("127.0.0.1", tcpPort)
    val writer = new OutputStreamWriter(socket.getOutputStream, UTF_8)
    writer.write("<show_state/>")
    writer.flush()
    assert(socket.getInputStream.read() == '<')
    assert(now() < periodEnd + 2.s)
    awaitSuccess(run.closed)
  }
}

private object JS1492IT {
  private val TestJobPath = JobPath("/test")
}
