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
import com.sos.scheduler.engine.test.SchedulerTestUtils.interceptSchedulerError
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class NodeOrderPluginIT extends FreeSpec with ScalaSchedulerTest {

  "New orders are added" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](DOrderKey) {
        eventBus.awaitingKeyedEvent[OrderFinishedEvent](COrderKey) {
          eventBus.awaitingKeyedEvent[OrderFinishedEvent](BOrderKey) {
            eventBus.awaitingKeyedEvent[OrderFinishedEvent](OriginalOrderKey) {
              scheduler executeXml OrderCommand(OriginalOrderKey)
            }
          }
        }
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

  "<add_order> must not denote the own job-chain" in {
    interceptSchedulerError(MessageCode("Z-JAVA-105")) {
      scheduler executeXml
        <job_chain name="test-own"
                   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                   xmlns:NodeOrderPlugin="https://jobscheduler-plugins.sos-berlin.com/NodeOrderPlugin">
          <job_chain_node state="100" job="/test-exit-0">
            <on_return_codes>
              <on_return_code return_code="0">
                <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-own"/>
              </on_return_code>
            </on_return_codes>
          </job_chain_node>
          <job_chain_node.end state="end"/>
        </job_chain>
    } .getMessage should include ("must denote the own job_chain")
  }
}

private object NodeOrderPluginIT {
  private val OriginalOrderKey = JobChainPath("/test-folder/a") orderKey "TEST"
  private val BOrderKey = JobChainPath("/test-folder/b") orderKey "TEST"  // Added by <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-b"/>
  private val COrderKey = JobChainPath("/test-folder-c/c") orderKey "TEST"  // Added by <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-c"/>
  private val DOrderKey = JobChainPath("/test-folder/d") orderKey "aaaTESTzzz"  // Added by <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-d" NodeOrderPlugin:id="aaa${ORDER_ID}zzz"/>
  private val ErrorOrderKey = JobChainPath("/test-folder/error") orderKey "TEST"
  private val MissingJobchainCode = MessageCode("SCHEDULER-161")
}
