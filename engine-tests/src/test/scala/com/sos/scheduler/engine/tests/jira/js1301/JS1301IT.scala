package com.sos.scheduler.engine.tests.jira.js1301

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.data.filebased.{FileBasedAddedEvent, FileBasedRemovedEvent}
import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1301.JS1301IT._
import java.nio.file.Files
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1301 &lt;job_chain process_class=""/>.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1301IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val agentHttpPort = FreeTcpPortFinder.findRandomFreeTcpPort()
  private lazy val agent = {
    val conf = AgentConfiguration(
      httpPort = agentHttpPort,
      httpInterfaceRestriction = Some("127.0.0.1"),
      environment = Map("TEST_AGENT" → "*AGENT*"))
    new Agent(conf).closeWithCloser
  }

  protected override def onSchedulerActivated(): Unit =
    awaitResult(agent.start(), 10.s)

  "Order changes to error state when job chain process class is missing" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-161"))) {
      runOrder(AJobChainPath orderKey "1").state shouldEqual OrderState("ERROR")
    }
  }

  "Job chain process class" in {
    testEnvironment.fileFromPath(AProcessClassPath).xml = <process_class remote_scheduler={s"http://127.0.0.1:$agentHttpPort"}/>
    testEnvironment.fileFromPath(BProcessClassPath).xml = <process_class remote_scheduler={s"http://127.0.0.1:$agentHttpPort"}/>
    instance[FolderSubsystem].updateFolders()
    val orderKey = AJobChainPath orderKey "2"
    runOrder(orderKey).state shouldEqual OrderState("END")
    orderLog(orderKey) should include ("SHELL TEST_AGENT=*AGENT*")
    orderLog(orderKey) should include ("API TEST_AGENT=*AGENT*")
  }

  "Deletion of a process class is delayed until all using tasks terminated" in {
    val orderKey = AJobChainPath orderKey "DELETE"
    val file = testEnvironment.fileFromPath(AProcessClassPath)
    val content = file.contentBytes
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
      eventBus.awaitingKeyedEvent[FileBasedAddedEvent](AProcessClassPath) {
        file.contentBytes = content
        instance[FolderSubsystem].updateFolders()
      }
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
    }
    .state shouldEqual OrderState("END")
  }

  "Same API task cannot be used by two job chains with different process classes" in {
    val aOrderKey = AJobChainPath orderKey "A-API"
    val bOrderKey = BJobChainPath orderKey "B-API"
    runOrder(aOrderKey).state shouldEqual OrderState("END")
    eventBus.awaitingEvent[TaskEndedEvent](_.jobPath == JavaJobPath) {
      interceptErrorLogEvents(Set(MessageCode("SCHEDULER-491"))) {
        eventBus.awaitingKeyedEvent[OrderStepEndedEvent](bOrderKey) {
          scheduler executeXml OrderCommand(bOrderKey)
        }
      }
    }
    assert(job(JavaJobPath).state == JobState.stopped)
    assert(order(bOrderKey).state == OrderState("100"))
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
