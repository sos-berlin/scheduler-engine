package com.sos.scheduler.engine.plugins.nodeorder

import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.plugins.nodeorder.NodeOrderPlugin._
import com.sos.scheduler.engine.plugins.nodeorder.NodeOrderPluginIT._
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class NodeOrderPluginIT extends FreeSpec with ScalaSchedulerTest {

  import controller.eventBus

  "Order is cloned" in {
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](ClonedOrderKey) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](OriginalOrderKey) {
        scheduler executeXml OrderCommand(OriginalOrderKey)
      }
    }
  }

  "Error when adding the new order is logged and ignored" in {
    controller.toleratingErrorCodes(Set(MissingJobchainCode, CommandFailedCode)) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](ErrorOrderKey) {
        autoClosing(controller.newEventPipe()) { eventPipe ⇒
          scheduler executeXml OrderCommand(ErrorOrderKey)
          eventPipe.nextWithCondition[ErrorLogEvent] { _.codeOption == Some(MissingJobchainCode) }
          eventPipe.nextWithCondition[ErrorLogEvent] { _.codeOption == Some(CommandFailedCode) }
        }
      }
    }
  }
}

private object NodeOrderPluginIT {
  private val OriginalOrderKey = JobChainPath("/test-a") orderKey "TEST"
  private val ClonedOrderKey = JobChainPath("/test-b") orderKey "TEST"  // Added by <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-b"/>
  private val ErrorOrderKey = JobChainPath("/test-error") orderKey "TEST"
  private val MissingJobchainCode = MessageCode("SCHEDULER-161")
}
