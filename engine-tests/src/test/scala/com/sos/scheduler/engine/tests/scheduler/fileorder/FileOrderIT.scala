package com.sos.scheduler.engine.tests.scheduler.fileorder

import com.google.common.io.Files.touch
import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.system.OperatingSystem.isWindows
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.Logged
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey, OrderStarted}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
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
final class FileOrderIT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List("-distributed-orders"))
  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]
  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()
  private val numbers = Iterator from 1
  private def newMatchingFile() = directory / s"X-MATCHING-FILE-${numbers.next()}"

  for ((withAgent, testGroupName) ← List(false → "Without Agent", true → "With Agent")) testGroupName - {
    lazy val agentUriOption = withAgent.option(agentUri)
    lazy val orderBlacklistedErrorSet = (!withAgent).option(MessageCode("SCHEDULER-340")).toSet
    lazy val notificationIsActive = withAgent || isWindows
    for ((isDistributed, testGroupName) ← List(false → "Not distributed", true → "Distributed")) testGroupName - {

      "(change ProcessClass and JobChain)" in {
        // Very long period ("repeat") to check directory change notification (not under Linux
        // But short period when agentFileExist does not applies, to allow check for file removal
        val repeat = if (withAgent && isDistributed || !notificationIsActive) 1.s else 1.h
        deleteAndWriteConfigurationFile(TestJobChainPath, makeJobChainElem(directory, DeleteJobPath, repeat = repeat, isDistributed = isDistributed))
        deleteAndWriteConfigurationFile(TestProcessClassPath, ProcessClassConfiguration(agentUris = agentUriOption.toList))
      }

      "Some files, one after the other" in {
        for (_ ← 1 to 3) {
          sleep(1.s)  // Delay until file order source has started next directory poll, to check directory change notification
          val matchingFile = newMatchingFile()
          val orderKey = TestJobChainPath orderKey matchingFile.toString
          runUntilFileRemovedMessage(orderKey) {
            eventBus.awaiting[OrderFinished](orderKey) {
              logger.info(s"touch $matchingFile")
              touch(matchingFile)
            }
          }
        }
      }

      "Change of first job definition does not disturb processing" in {
        // Needs the process class defined in previous test
        val matchingFile = newMatchingFile()
        val orderKey = TestJobChainPath orderKey matchingFile.toString
        runUntilFileRemovedMessage(orderKey) {
          eventBus.awaiting[OrderFinished](orderKey) {
            testEnvironment.fileFromPath(DeleteJobPath).append(" ")
            instance[FolderSubsystemClient].updateFolders()
            touch(matchingFile)
          }
        }
      }

      "A file, not removed by job chain, stays blacklisted until removed later" in {
        logger.info(s"Test: $testName")
        val repeat = 1.s
        val delay = repeat dividedBy 2
        deleteAndWriteConfigurationFile(TestJobChainPath, makeJobChainElem(directory, JobPath("/test-dont-delete"), repeat, isDistributed = isDistributed))
        val file = directory / "X-MATCHING-TEST-DONT-DELETE"
        val orderKey = TestJobChainPath orderKey file.toString
        controller.toleratingErrorCodes(orderBlacklistedErrorSet) {
          runUntilFileRemovedMessage(orderKey) {
            eventBus.awaiting[OrderFinished](orderKey) {
              touch(file)
            }
            val startedAgain = eventBus.eventFuture[OrderStarted.type](orderKey)
            assert(orderIsBlacklisted(orderKey))
            sleep(repeat + delay)
            assert(orderIsBlacklisted(orderKey))
            assert(!startedAgain.isCompleted)
            logger.info(s"delete $file")
            delete(file)
          }
        }
        assert(!orderExists(orderKey))
        controller.toleratingErrorCodes(orderBlacklistedErrorSet) {
          runUntilFileRemovedMessage(orderKey) {
            eventBus.awaiting[OrderFinished](orderKey) {
              val started = eventBus.eventFuture[OrderStarted.type](orderKey)
              assert(!started.isCompleted)
              sleep(repeat + delay)
              assert(!started.isCompleted)
              touch(file)
            }
            assert(orderIsBlacklisted(orderKey))
            logger.info(s"delete $file")
            delete(file)
          }
        }
      }
    }

    "regex filters files" in {
      val repeat = if (!notificationIsActive) 1.s else 1.h
      deleteAndWriteConfigurationFile(TestJobChainPath, makeJobChainElem(directory, JobPath("/test-delete"), repeat = repeat, isDistributed = false))
      val matchingFile = newMatchingFile()
      val ignoredFile = directory / "IGNORED-FILE"
      List(matchingFile, ignoredFile) foreach touch
      val List(matchingOrderKey, ignoredOrderKey) = List(matchingFile, ignoredFile) map { TestJobChainPath orderKey _.toString }
      val ignoredStarted = eventBus.eventFuture[OrderStarted.type](ignoredOrderKey)
      controller.toleratingErrorCodes(orderBlacklistedErrorSet) {
        runUntilFileRemovedMessage(matchingOrderKey) {
          eventBus.awaiting[OrderFinished](matchingOrderKey) {
            touch(matchingFile)
          }
        }
      }
      sleep(2.s)
      assert(!ignoredStarted.isCompleted)
    }

    "Ordinary job is still startable" in {
      runJob(JobPath("/test-ordinary"))
    }
  }

  private def runUntilFileRemovedMessage(orderKey: OrderKey)(body: ⇒ Unit): Unit =
    eventBus.awaitingWhen[Logged](_.event.codeOption contains MessageCode("SCHEDULER-981")) { // "File has been removed"
      body
    }
}

private object FileOrderIT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestProcessClassPath = ProcessClassPath("/test")
  private val DeleteJobPath = JobPath("/test-delete")
  private val logger = Logger(getClass)

  private def makeJobChainElem(directory: Path, jobPath: JobPath, repeat: Duration, isDistributed: Boolean): xml.Elem =
    <job_chain distributed={isDistributed.toString} process_class={TestProcessClassPath.withoutStartingSlash}>
      <file_order_source directory={directory.toString} regex="MATCHING-" repeat={repeat.getSeconds.toString}/>
      <job_chain_node state="100" job={jobPath.string}/>
      <job_chain_node.end state="END"/>
    </job_chain>
}
