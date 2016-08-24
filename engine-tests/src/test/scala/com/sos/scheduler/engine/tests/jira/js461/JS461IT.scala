package com.sos.scheduler.engine.tests.jira.js461

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderResumed, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-461: modify order set state to endstate.
  *
  * The sample configuration contains a jobchain with three nodes.
  * Running this test the chain starts and should be suspend at the second node
  * (job js-461-2), because the job ends with error. The test set the state of
  * the suspended order to "END" and resumed it.
  * The test expects the following events fired by the scheduler:
  * - EventOrderSuspended if the job job js-461-2 ends with error
  * - EventOrderResumed if the order was set to suspended="no"
  * - EventOrderFinished because the order resumed in the "END" state
  */
@RunWith(classOf[JUnitRunner])
final class JS461IT extends FreeSpec with ScalaSchedulerTest {

  private val testOrderKey = JobChainPath("/js-461") orderKey "js-461"

  "test" in  {
    controller.toleratingErrorCodes(_ ⇒ true) {
      eventBus.awaitingKeyedEvent[OrderSuspended.type](testOrderKey) {
        scheduler executeXml OrderCommand(testOrderKey)
      }
      eventBus.awaitingKeyedEvent[OrderFinished](testOrderKey) {
        scheduler executeXml ModifyOrderCommand(testOrderKey, nodeId = Some(NodeId("END")))
      }
      // Funny, the order, being blacklisted, is resumed after it has been finished
      eventBus.awaitingKeyedEvent[OrderResumed.type](testOrderKey) {
        scheduler executeXml ModifyOrderCommand(testOrderKey, suspended = Some(false))
      }
    }
  }
}
