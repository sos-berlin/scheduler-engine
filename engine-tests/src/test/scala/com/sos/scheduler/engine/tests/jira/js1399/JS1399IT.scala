package com.sos.scheduler.engine.tests.jira.js1399

import com.google.common.io.Files.touch
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderStarted}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.ImplicitTimeout
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1399.JS1399IT._
import java.net.{InetSocketAddress, ServerSocket}
import java.nio.file.Files.exists
import java.nio.file.Path
import java.time.Duration
import java.util.concurrent.TimeoutException
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1399IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()

  "file_order_source waits for process_class" in {
    val matchingFile = directory / "X-MATCHING-FILE-1"
    val orderKey = TestJobChainPath orderKey matchingFile.toString
    writeConfigurationFile(TestJobChainPath, newJobChainElem(directory, 1.s))
    eventBus.awaiting[OrderFinished](orderKey) {
      intercept[TimeoutException] {
        implicit val implicitTimeout = ImplicitTimeout(10.s)
        eventBus.awaiting[OrderStarted.type](orderKey) {
          assert(!exists(matchingFile))
          touch(matchingFile)
        }
      }
      writeConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = List(agentUri)))
    }
    assert(!exists(matchingFile))
  }

  "file_order_source stops when process_class is to be removed, and restarts when process_class is added" in {
    val matchingFile = directory / "X-MATCHING-FILE-2"
    val orderKey = TestJobChainPath orderKey matchingFile.toString
    deleteConfigurationFile(TestProcessClassPath)
    eventBus.awaiting[OrderFinished](orderKey) {
      intercept[TimeoutException] {
        implicit val implicitTimeout = ImplicitTimeout(10.s)
        eventBus.awaiting[OrderStarted.type](orderKey) {
          assert(!exists(matchingFile))
          touch(matchingFile)
        }
      }
      writeConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = List(agentUri)))
    }
    assert(!exists(matchingFile))
  }

  "file_order_source handles change of process_class" in {
    val matchingFile = directory / "X-MATCHING-FILE-3"
    val orderKey = TestJobChainPath orderKey matchingFile.toString
    controller.toleratingErrorLogged(_.message contains "spray.can.Http$ConnectionException") {
      autoClosing(new ServerSocket()) { socket ⇒
        socket.bind(new InetSocketAddress("127.0.0.1", 0))
        val deadPort = socket.getLocalPort
        writeConfigurationFile(TestProcessClassPath,
          ProcessClassConfiguration(agentUris = List(AgentAddress(s"http://127.0.0.1:$deadPort"))))
        intercept[TimeoutException] {
          implicit val implicitTimeout = ImplicitTimeout(10.s)
          eventBus.awaiting[OrderStarted.type](orderKey) {
            assert(!exists(matchingFile))
            touch(matchingFile)
          }
        }
      }
      deleteConfigurationFile(TestJobChainPath)  // Close to get HTTP error while we tolerate it
    }
  }

  private def newJobChainElem(directory: Path, repeat: Duration): xml.Elem =
    <job_chain file_watching_process_class={TestProcessClassPath.withoutStartingSlash}>
      <file_order_source directory={directory.toString} regex="MATCHING-" repeat={repeat.getSeconds.toString}/>
      <job_chain_node state="100" job="/test"/>
      <file_order_sink state="END" remove="true"/>
    </job_chain>
}

private object JS1399IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestProcessClassPath = ProcessClassPath("/test")
}
