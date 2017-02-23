package com.sos.scheduler.engine.tests.scheduler.fileorder

import com.google.common.io.Files.touch
import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.log.Logged
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.{ProcessClassConfiguration, RemoveOrderCommand}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.fileorder.FileOrderSinkIT._
import java.nio.file.{Files, Path}
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * Tests file_order_source: with and without agent, is distributed or not distributed, not-removed files.
 * <p>
 * JS-1300 &lt;file_order_source remote_scheduler="..">
 * <p>
 * JS-1398 Blacklisted file orders should be removed when corresponding file has been removed
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class FileOrderSinkIT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List("-distributed-orders"),
    ignoreError = Set(MessageCode("SCHEDULER-291")))  // !!! Nach einem Lauf ohne Agenten kann der Lauf mit Agenten das Auftragsprotokoll nicht löschen (in Order::close) - Es sei noch in Gebrauch.
  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]
  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()
  private lazy val moveToDirectory = testEnvironment.newFileOrderSourceDirectory()
  private val numbers = Iterator from 1
  private def newMatchingFile() = directory / s"X-MATCHING-FILE-${numbers.next()}"

  for ((withAgent, testGroupName) ← List(false → "Without Agent", true → "With Agent")) testGroupName - {
    lazy val agentUriOption = withAgent.option(agentUri)

    for ((isDistributed, testGroupName) ← List(false → "Not distributed", true → "Distributed")) testGroupName - {
      "(change ProcessClass)" in {
        deleteAndWriteConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = agentUriOption.toList))
      }

      "file_order_sink remove" in {
        deleteAndWriteConfigurationFile(TestJobChainPath,
          <job_chain distributed={isDistributed.toString} process_class={TestProcessClassPath.withoutStartingSlash}>
            <file_order_source directory={directory.toString} regex="MATCHING-" repeat="1"/>
            <job_chain_node state="100" job="/test-dont-delete"/>
            <file_order_sink state="SINK" remove="true"/>
          </job_chain>)
        val file = newMatchingFile()
        runFile(file)
        assert(!Files.exists(file))
      }

      "file_order_sink move_to" in {
        deleteAndWriteConfigurationFile(TestJobChainPath,
          <job_chain distributed={isDistributed.toString} process_class={TestProcessClassPath.withoutStartingSlash}>
            <file_order_source directory={directory.toString} regex="MATCHING-" repeat="1"/>
            <job_chain_node state="100" job="/test-dont-delete"/>
            <file_order_sink state="SINK" move_to={moveToDirectory.toString}/>
          </job_chain>)
        val file = newMatchingFile()
        val movedFile = moveToDirectory resolve file.getFileName
        runFile(file, "FIRST FILE")
        assert(!Files.exists(file))
        assert(Files.exists(movedFile))
        assert(movedFile.contentString == "FIRST FILE")

        // <file_order_sink move_to=".." overwrites existing file in directory
        runFile(file, "SECOND FILE")
        assert(!Files.exists(file))
        assert(Files.exists(movedFile))
        assert(movedFile.contentString == "SECOND FILE")
      }

      "file_order_sink move_to must denote a directory" in {
        val moveTo = directory resolve "moved"
        deleteAndWriteConfigurationFile(TestJobChainPath,
          <job_chain distributed={isDistributed.toString} process_class={TestProcessClassPath.withoutStartingSlash}>
            <file_order_source directory={directory.toString} regex="MATCHING-" repeat="1"/>
            <job_chain_node state="100" job="/test-dont-delete"/>
            <file_order_sink state="SINK" move_to={moveTo.toString}/>
          </job_chain>)
        val file = newMatchingFile()
        sleep(1.s)  // Delay until file order source has started next directory poll, to check directory change notification
        val orderKey = TestJobChainPath orderKey file.toString
        controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-340"))) {  // "File still exists. Order has been set on the blacklist"
          eventBus.awaiting[OrderFinished](orderKey) {
            touch(file)
          }
        }
        assert(orderIsBlacklisted(orderKey))
        scheduler executeXml RemoveOrderCommand(orderKey)
        assert(Files.exists(file))
        // ??? Pausenlose RequestFileOrderSource, wenn wir die Datei nicht löschen.
        Files.delete(file)
      }
    }
  }

  private def runFile(matchingFile: Path, content: String = ""): Unit = {
    sleep(1.s)  // Delay until file order source has started next directory poll, to check directory change notification
    val orderKey = TestJobChainPath orderKey matchingFile.toString
    runUntilFileRemovedMessage(orderKey) {
      val run = OrderRun(orderKey)
      matchingFile.contentString = content
      val result = run.result await TestTimeout
      assert(result.nodeId == NodeId("SINK"))   // JS-1627 <file_order_sink> must not changed order nodeId
    }
  }

  private def runUntilFileRemovedMessage(orderKey: OrderKey)(body: ⇒ Unit): Unit =
    eventBus.awaitingWhen[Logged](_.event.codeOption contains MessageCode("SCHEDULER-981")) { // "File has been removed"
      body
    }
}

private object FileOrderSinkIT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestProcessClassPath = ProcessClassPath("/test")
}
