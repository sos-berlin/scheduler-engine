package com.sos.scheduler.engine.tests.order.reset

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderResumed, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand.Action
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class OrderResetIT extends FreeSpec with ScalaSchedulerTest {

  private val testOrderKey = JobChainPath("/chain") orderKey "test"

  "test" in {
    controller.toleratingErrorCodes(_ ⇒ true) {
      eventBus.awaitingKeyedEvent[OrderSuspended.type](testOrderKey) {
        scheduler executeXml OrderCommand(testOrderKey)
      }
      eventBus.awaitingKeyedEvent[OrderResumed.type](testOrderKey) {
        scheduler executeXml ModifyOrderCommand(testOrderKey, action = Some(Action.reset))
      }
    }
  }
}
