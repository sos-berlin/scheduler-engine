package com.sos.scheduler.engine.tests.jira.js1003

import JS1003IT._
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.order.jobchain.JobChainNodeAction
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand.Action
import com.sos.scheduler.engine.data.xmlcommands.{OrderCommand, ModifyOrderCommand}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1003IT extends FunSuite with ScalaSchedulerTest {

  private lazy val orderSubsystem = instance[OrderSubsystem]
  private lazy val jobChain = orderSubsystem.jobChain(testJobChainPath)

  test("(prepare job chain)") {
    jobChain.node(state100).action = JobChainNodeAction.nextState
    jobChain.node(state300).action = JobChainNodeAction.stop
  }

  test("Resetting standing order should restart suspended order") {
    checkBehaviour(standingOrderKey) {
      scheduler executeXml ModifyOrderCommand(standingOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  test("Reset non-standing order should restart suspended order") {
    checkBehaviour(testOrderKey) {
      scheduler executeXml OrderCommand(testOrderKey)
    }
  }

  private def checkBehaviour(orderKey: OrderKey)(startOrder: => Unit) {
    val eventPipe = controller.newEventPipe()
    startOrder
    if (orderKey != standingOrderKey)  // Ein Dauerauftrag überspringt den ersten Knoten ohne Event
      eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState should equal (state100)
    eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState should equal (state200)
    def order = orderSubsystem.order(orderKey)
    order.getState should be (state300)
    order.setSuspended(true)
    order should be ('suspended)
    scheduler executeXml ModifyOrderCommand(orderKey, action = Some(Action.reset))
    order should not be 'suspended
    eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState should equal (state300)
//    Alternative, wir im Ticket JS-1003 erwartet, aber entgegen der Dokumentation und dem tatsächlichen Verhalten:
//    order should be ('suspended)
//    order.getState should be (state200)
//    withClue("Order should not start because it is suspended:") { intercept[TimeoutException] { eventPipe.next[OrderStepStartedEvent](orderKey, timeout = 5.s) } }
//    order.getState should be (state200)
    eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState should equal (state200)
  }
}

object JS1003IT {
  private val testJobChainPath = JobChainPath.of("/test")
  private val state100 = OrderState("100")
  private val state200 = OrderState("200")
  private val state300 = OrderState("300")
  private val standingOrderKey = testJobChainPath orderKey "1"
  private val testOrderKey = testJobChainPath orderKey "2"
}
