package com.sos.scheduler.engine.tests.jira.js1301

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.filebased.FileBasedRemoved
import com.sos.scheduler.engine.data.job.{JobPath, JobState, TaskEnded, TaskStarted}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.log.InfoLogged
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand, ProcessClassConfiguration}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1301.JS1301IT._
import java.nio.file.Files.delete
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

  override protected def newAgentConfiguration() = super.newAgentConfiguration().copy(environment = Map("TEST_AGENT" → "*AGENT*"))

  "Order changes to error nodeId when job chain process class is missing" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-161"))) {
      runOrder(AJobChainPath orderKey "1").nodeId shouldEqual NodeId("ERROR")
    }
  }

  "Job chain process class" in {
    writeConfigurationFile(AProcessClassPath, ProcessClassConfiguration(agentUris = List(agentUri)))
    writeConfigurationFile(BProcessClassPath, ProcessClassConfiguration(agentUris = List(agentUri)))
    val orderKey = AJobChainPath orderKey "2"
    runOrder(orderKey).nodeId shouldEqual NodeId("END")
    orderLog(orderKey) should include ("API TEST_AGENT=/*AGENT*/")
    orderLog(orderKey) should include ("SHELL TEST_1_AGENT=/*AGENT*/")
    orderLog(orderKey) should include ("SHELL TEST_2_AGENT=//")
  }

  "Deletion of a process class is delayed until all using tasks terminated" in {
    val orderKey = AJobChainPath orderKey "DELETE"
    val file = testEnvironment.fileFromPath(AProcessClassPath)
    val fileContent = file.xml
    withEventPipe { events ⇒
      scheduler executeXml OrderCommand(orderKey, parameters = Map("sleep" → "5"))
      events.next[OrderStepStarted](orderKey)
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(true))
      sleep(2.s)
      eventBus.awaitingWhen[InfoLogged](_.event.codeOption contains MessageCode("SCHEDULER-989")) { // "Process_class cannot be removed now, it will be done later"
        delete(file)
        instance[FolderSubsystemClient].updateFolders()
      }
      events.nextWhen[TaskEnded](_.key.jobPath == JavaJobPath, 10.s)
      events.next[FileBasedRemoved.type](AProcessClassPath)
      assert(orderOverview(orderKey).nodeId == NodeId("200"))
      writeConfigurationFile(AProcessClassPath, fileContent)
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
      events.next[OrderFinished](orderKey)
    }
    .nodeId shouldEqual NodeId("END")
  }

  "For a second order requiring a different process class, the running task is terminated" in {
    val aOrderKey = AJobChainPath orderKey "A-API"
    val bOrderKey = BJobChainPath orderKey "B-API"
    runOrder(aOrderKey).nodeId shouldEqual NodeId("END")
    withEventPipe { events ⇒
      scheduler executeXml OrderCommand(bOrderKey)
      events.nextWhen[InfoLogged](_.event.codeOption contains MessageCode("SCHEDULER-271"))   // "Task is being terminated in favour of ..."
      events.nextWhen[TaskEnded](_.key.jobPath == JavaJobPath)
      events.nextWhen[TaskStarted.type](_.key.jobPath == JavaJobPath)
      events.next[OrderFinished](bOrderKey).nodeId shouldBe NodeId("END")
    }
    assert(jobOverview(JavaJobPath).state == JobState.running)
  }

  private def runOrder(orderKey: OrderKey): OrderFinished =
    eventBus.awaiting[OrderFinished](orderKey) {
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
