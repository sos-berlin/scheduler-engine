package com.sos.scheduler.engine.tests.jira.js1301

import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.filebased.FileBasedRemovedEvent
import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent, TaskStartedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand, ProcessClassConfiguration}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1301.JS1301IT._
import java.nio.file.Files
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1301 &lt;job_chain process_class=""/>.
 * JS-1450 Job's process class is respected before job chain's process class
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1301IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected lazy val agentConfiguration = AgentConfiguration.forTest().copy(environment = Map("TEST_AGENT" → "*AGENT*"))

  "Order changes to error state when job chain process class is missing" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-161"))) {
      runOrder(AJobChainPath orderKey "1").state shouldEqual OrderState("ERROR")
    }
  }

  "Job chain process class" in {
    writeConfigurationFile(AProcessClassPath, ProcessClassConfiguration(agentUris = List(agentUri)))
    writeConfigurationFile(BProcessClassPath, ProcessClassConfiguration(agentUris = List(agentUri)))
    val orderKey = AJobChainPath orderKey "2"
    runOrder(orderKey).state shouldEqual OrderState("END")
    orderLog(orderKey) should include ("API TEST_AGENT=/*AGENT*/")
    orderLog(orderKey) should include ("SHELL TEST_1_AGENT=/*AGENT*/")
    orderLog(orderKey) should include ("SHELL TEST_2_AGENT=//")
  }

  "Deletion of a process class is delayed until all using tasks terminated" in {
    val orderKey = AJobChainPath orderKey "DELETE"
    val file = testEnvironment.fileFromPath(AProcessClassPath)
    val fileContent = file.xml
    eventBus.awaitingKeyedEvent[FileBasedRemovedEvent](AProcessClassPath) {
      eventBus.awaitingEvent2[TaskEndedEvent](predicate = _.jobPath == JavaJobPath, timeout = 10.s) {
        eventBus.awaitingKeyedEvent[OrderStepStartedEvent](orderKey) {
          scheduler executeXml OrderCommand(orderKey, parameters = Map("sleep" → "5"))
        }
        scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(true))
        sleep(2.s)
        eventBus.awaitingEvent[InfoLogEvent](_.codeOption == Some(MessageCode("SCHEDULER-989"))) { // "Process_class cannot be removed now, it will be done later"
          Files.delete(file)
          instance[FolderSubsystem].updateFolders()
        }
      }
    }
    assert(order(orderKey).state == OrderState("200"))
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
      writeConfigurationFile(AProcessClassPath, fileContent)
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
    }
    .state shouldEqual OrderState("END")
  }

  "For a second order requiring a different process class, the running task is terminated" in {
    val aOrderKey = AJobChainPath orderKey "A-API"
    val bOrderKey = BJobChainPath orderKey "B-API"
    runOrder(aOrderKey).state shouldEqual OrderState("END")
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](bOrderKey) {
      eventBus.awaitingEvent[TaskStartedEvent](_.jobPath == JavaJobPath) {
        eventBus.awaitingEvent[TaskEndedEvent](_.jobPath == JavaJobPath) {
          scheduler executeXml OrderCommand(bOrderKey)
        }
      }
    } .state shouldBe OrderState("END")
    assert(job(JavaJobPath).state == JobState.running)
  }

  private def runOrder(orderKey: OrderKey): OrderFinishedEvent =
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
      scheduler executeXml OrderCommand(orderKey)
    }
}

private object JS1301IT {
  private val AJobChainPath = JobChainPath("/test-folder/test-a")
  private val BJobChainPath = JobChainPath("/test-folder/test-b")
  private val AProcessClassPath = ProcessClassPath("/test-folder/test-a")
  private val BProcessClassPath = ProcessClassPath("/test-folder/test-b")
  private val JavaJobPath = JobPath("/test-folder/test-java")
}
