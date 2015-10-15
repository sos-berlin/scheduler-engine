package com.sos.scheduler.engine.tests.jira.js1492

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.filebased.FileBasedAddedEvent
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1492.JS1492IT._
import java.io.OutputStreamWriter
import java.net.Socket
import java.nio.charset.StandardCharsets.UTF_8
import org.joda.time.DateTimeZone
import org.joda.time.Instant.now
import org.joda.time.format.DateTimeFormat
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

  import controller.eventBus
  
  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))

  "JS-1492" in {
    val periodEnd = now() + 3.s
    val expectedTaskEnd = now() + 5.s
    eventBus.awaitingKeyedEvent[FileBasedAddedEvent](TestJobPath) {
      testEnvironment.fileFromPath(TestJobPath).contentString =
        <job>
          <script language="shell">{
            if (isWindows) "ping -n 6 127.0.0.1 >nul" else "sleep 5"
          }</script>
          <run_time>
            <period end={DateTimeFormat forPattern "HH:mm:ss" withZone DateTimeZone.getDefault print periodEnd}/>
          </run_time>
        </job>.toString
      instance[FolderSubsystem].updateFolders()
    }
    val orderKey = TestJobChainPath orderKey "1"
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
      scheduler executeXml OrderCommand(orderKey)
      sleep(periodEnd + 100.ms - now())
      checkTcp()
      assert(now() < expectedTaskEnd - 1.s)
    }
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
  private val TestJobChainPath = JobChainPath("/test")
  private val TestJobPath = JobPath("/test")
}
