package com.sos.scheduler.engine.tests.scheduler.fileorder

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.LogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey, OrderTouchedEvent}
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.fileorder.FileOrderIT._
import java.nio.file.Files.delete
import java.nio.file.Path
import java.time.Duration
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
final class FileOrderIT extends FreeSpec with ScalaSchedulerTest with AgentTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List("-distributed-orders"))
  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]
  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()
  private lazy val matchingFile = directory / "X-MATCHING-FILE"

  for ((withAgent, testGroupName) ← List(true → "With Agent", false → "Without Agent")) testGroupName - {
    lazy val agentUriOption = withAgent.option(agentUri)
    lazy val orderSetOnBlacklistErrorSet = (!withAgent).option(MessageCode("SCHEDULER-340")).toSet
    for ((isDistributed, testGroupName) ← List(false → "Not distributed", true → "Distributed")) testGroupName - {

      "Some files, one after the other" in {
        // Very long period ("repeat") to check directory change notification (not under Linux
        // But short period when agentFileExist does not applies, to allow check for file removal
        val notificationIsActive = withAgent || isWindows
        val repeat = if (withAgent && isDistributed || !notificationIsActive) 1.s else 1.h
        scheduler executeXml newJobChainElem(directory, agentUri = agentUriOption, JobPath("/test-delete"), repeat = repeat, isDistributed = isDistributed)
        for (_ ← 1 to 3) {
          sleep(1.s)  // Delay until file order source has started next directory poll, to check directory change notification
          val orderKey = TestJobChainPath orderKey matchingFile.toString
          runUntilFileRemovedMessage(orderKey) {
            eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
              touch(matchingFile)
            }
          }
        }
      }

      "A file, not removed by job chain, stays on blacklist until removed later" in {
        val repeat = 1.s
        val delay = repeat dividedBy 2
        scheduler executeXml newJobChainElem(directory, agentUri = agentUriOption, JobPath("/test-dont-delete"), repeat, isDistributed = isDistributed)
        val file = directory / "X-MATCHING-TEST-DONT-DELETE"
        val orderKey = TestJobChainPath orderKey file.toString
        controller.toleratingErrorCodes(orderSetOnBlacklistErrorSet) {
          runUntilFileRemovedMessage(orderKey) {
            eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
              touch(file)
            }
            val startedAgain = eventBus.keyedEventFuture[OrderTouchedEvent](orderKey)
            assert(orderIsOnBlacklist(orderKey))
            sleep(repeat + delay)
            assert(orderIsOnBlacklist(orderKey))
            assert(!startedAgain.isCompleted)
            delete(file)
          }
        }
        assert(!orderExists(orderKey))
        controller.toleratingErrorCodes(orderSetOnBlacklistErrorSet) {
          runUntilFileRemovedMessage(orderKey) {
            eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
              val started = eventBus.keyedEventFuture[OrderTouchedEvent](orderKey)
              assert(!started.isCompleted)
              sleep(repeat + delay)
              assert(!started.isCompleted)
              touch(file)
            }
            assert(orderIsOnBlacklist(orderKey))
            delete(file)
          }
        }
      }
    }

    "regex filters files" in {
      scheduler executeXml newJobChainElem(directory, agentUri = agentUriOption, JobPath("/test-delete"), repeat = 1.h, isDistributed = false)
      val ignoredFile = directory / "IGNORED-FILE"
      List(matchingFile, ignoredFile) foreach touch
      val List(matchingOrderKey, ignoredOrderKey) = List(matchingFile, ignoredFile) map { TestJobChainPath orderKey _.toString }
      val ignoredStarted = eventBus.keyedEventFuture[OrderTouchedEvent](ignoredOrderKey)
      controller.toleratingErrorCodes(orderSetOnBlacklistErrorSet) {
        runUntilFileRemovedMessage(matchingOrderKey) {
          eventBus.awaitingKeyedEvent[OrderFinishedEvent](matchingOrderKey) {
            touch(matchingFile)
          }
        }
      }
      sleep(2.s)
      assert(!ignoredStarted.isCompleted)
    }
  }

  private def runUntilFileRemovedMessage(orderKey: OrderKey)(body: ⇒ Unit): Unit =
    eventBus.awaitingEvent[LogEvent](_.codeOption contains MessageCode("SCHEDULER-981")) { // "File has been removed"
      body
    }

  private def orderIsOnBlacklist(orderKey: OrderKey): Boolean =
    if (jobChain(orderKey.jobChainPath).isDistributed)
      transaction { implicit entityManager ⇒
        val e = instance[HibernateOrderStore].fetch(orderKey)
        e.isOnBlacklist
      }
    else
      order(orderKey).isOnBlacklist
}

private object FileOrderIT {
  private val TestJobChainPath = JobChainPath("/test")

  private def newJobChainElem(directory: Path, agentUri: Option[String], jobPath: JobPath, repeat: Duration, isDistributed: Boolean): xml.Elem =
    <job_chain name={TestJobChainPath.withoutStartingSlash} distributed={isDistributed.toString}>
      <file_order_source directory={directory.toString} regex="MATCHING-" remote_scheduler={agentUri.orNull} repeat={repeat.getSeconds.toString}/>
      <job_chain_node state="100" job={jobPath.string}/>
      <job_chain_node.end state="END"/>
    </job_chain>
}
