package com.sos.scheduler.engine.tests.jira.js864

import JS864IT._
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class JS864IT extends ScalaSchedulerTest {

  private var eventPipe: EventPipe = _

  override def checkedBeforeAll() {
    controller.activateScheduler()
    eventPipe = controller.newEventPipe()
    for (i <- 0 to 5)
      scheduler executeXml newOrderElem(new OrderId(i.toString))
  }

  test("0) No job chain node having action='process'") {
    val orderKey = jobChainPath.orderKey(new OrderId("0"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should equal (new OrderTouchedEvent(orderKey))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, aState))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, bState))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should equal (new OrderFinishedEvent(orderKey))
  }

  test("1) Job chain node B has action='next_state'") {
    modifyNode(bState, nextStateAction)
    val orderKey = jobChainPath.orderKey(new OrderId("1"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should equal (new OrderTouchedEvent(orderKey))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, aState))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should equal (new OrderFinishedEvent(orderKey))
  }

  test("2) Job chain node A has action='next_state'") {
    modifyNode(bState, processAction)
    modifyNode(aState, nextStateAction)   // Alle wartenden Auftrage wechseln zu B
    val orderKey = jobChainPath.orderKey(new OrderId("2"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should equal (new OrderTouchedEvent(orderKey))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, bState))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should equal (new OrderFinishedEvent(orderKey))
  }

  test("3) Again, all job chain nodes having action='process'") {
    modifyNode(aState, processAction)
    val orderKey = jobChainPath.orderKey(new OrderId("3"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should equal (new OrderTouchedEvent(orderKey))
    // aState nicht, weil next_state im vorangehenden Test den Auftrag schon weitergeschoben hat.
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, bState))
    nextOrderEvent(orderKey) should equal (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should equal (new OrderFinishedEvent(orderKey))
  }

  test("4) All job chain nodes having action='next_state'") {
    modifyNode(aState, nextStateAction)
    modifyNode(bState, nextStateAction)
    modifyNode(cState, nextStateAction)
    val orderKey = jobChainPath.orderKey(new OrderId("4"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should equal (new OrderFinishedEvent(orderKey))
  }

  test("5) Again, no job chain node having action='next_state'") {
    modifyNode(bState, "process")
    val orderKey = jobChainPath.orderKey(new OrderId("5"))
    resumeOrder(orderKey)
    //intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition(2.s) { e: OrderTouchedEvent => e.getKey == jobChainPath.orderKey(orderId) } }
    nextOrderEvent(orderKey) should equal (new OrderFinishedEvent(orderKey))
  }

  private def nextOrderEvent(orderKey: OrderKey) =
    eventPipe.nextWithCondition { e: OrderEvent => e.orderKey == orderKey && isRelevantOrderEventClass(e.getClass) }

  private def isRelevantOrderEventClass(eventClass: Class[_ <: OrderEvent]) =
    List(classOf[OrderTouchedEvent], classOf[OrderFinishedEvent], classOf[OrderStepStartedEvent]) exists { _ isAssignableFrom eventClass }

  private def modifyNode(state: OrderState, action: String) {
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state={state.string} action={action}/>
  }

  private def resumeOrder(o: OrderKey) {
    scheduler executeXml <modify_order job_chain={o.jobChainPathString} order={o.idString} suspended="false"/>
  }
}

private object JS864IT {
  val jobChainPath = JobChainPath.of("/test")
  val aState = OrderState("A")
  val bState = OrderState("B")
  val cState = OrderState("C")
  val processAction = "process"
  val nextStateAction = "next_state"

  def newOrderElem(orderId: OrderId) =
    <order job_chain={jobChainPath.string} id={orderId.string} suspended="true">
      <params>
        <param name="count" value="0"/>
      </params>
    </order>

  class OrderTest(orderNumber: Int) {
    protected val orderId = new OrderId(orderNumber.toString)
  }
}
