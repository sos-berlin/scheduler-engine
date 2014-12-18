package com.sos.scheduler.engine.tests.jira.js864

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js864.JS864IT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS864IT extends FunSuite with ScalaSchedulerTest {

  private lazy val eventPipe = controller.newEventPipe()

  override def onBeforeSchedulerActivation(): Unit = {
    eventPipe
  }

  override def onSchedulerActivated(): Unit = {
    for (i <- 0 to 5)
      scheduler executeXml newOrderElem(OrderId(i.toString))
  }

  test("0) No job chain node having action='process'") {
    val orderKey = jobChainPath.orderKey(OrderId("0"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should be (new OrderTouchedEvent(orderKey))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, aState))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, bState))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should be (new OrderFinishedEvent(orderKey))
  }

  test("1) Job chain node B has action='next_state'") {
    modifyNode(bState, nextStateAction)
    val orderKey = jobChainPath.orderKey(OrderId("1"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should be (new OrderTouchedEvent(orderKey))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, aState))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should be (new OrderFinishedEvent(orderKey))
  }

  test("2) Job chain node A has action='next_state'") {
    modifyNode(bState, processAction)
    modifyNode(aState, nextStateAction)   // Alle wartenden Auftrage wechseln zu B
    val orderKey = jobChainPath.orderKey(OrderId("2"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should be (new OrderTouchedEvent(orderKey))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, bState))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should be (new OrderFinishedEvent(orderKey))
  }

  test("3) Again, all job chain nodes having action='process'") {
    modifyNode(aState, processAction)
    val orderKey = jobChainPath.orderKey(OrderId("3"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should be (new OrderTouchedEvent(orderKey))
    // aState nicht, weil next_state im vorangehenden Test den Auftrag schon weitergeschoben hat.
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, bState))
    nextOrderEvent(orderKey) should be (new OrderStepStartedEvent(orderKey, cState))
    nextOrderEvent(orderKey) should be (new OrderFinishedEvent(orderKey))
  }

  test("4) All job chain nodes having action='next_state'") {
    modifyNode(aState, nextStateAction)
    modifyNode(bState, nextStateAction)
    modifyNode(cState, nextStateAction)
    val orderKey = jobChainPath.orderKey(OrderId("4"))
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) should be (new OrderFinishedEvent(orderKey))
  }

  test("5) Again, no job chain node having action='next_state'") {
    modifyNode(bState, "process")
    val orderKey = jobChainPath.orderKey(OrderId("5"))
    resumeOrder(orderKey)
    //intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition(2.s) { e: OrderTouchedEvent => e.getKey == jobChainPath.orderKey(orderId) } }
    nextOrderEvent(orderKey) should be (new OrderFinishedEvent(orderKey))
  }

  private def nextOrderEvent(orderKey: OrderKey) =
    eventPipe.nextWithCondition { e: OrderEvent => e.orderKey == orderKey && isRelevantOrderEventClass(e.getClass) }

  private def isRelevantOrderEventClass(eventClass: Class[_ <: OrderEvent]) =
    List(classOf[OrderTouchedEvent], classOf[OrderFinishedEvent], classOf[OrderStepStartedEvent]) exists { _ isAssignableFrom eventClass }

  private def modifyNode(state: OrderState, action: String): Unit = {
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state={state.string} action={action}/>
  }

  private def resumeOrder(o: OrderKey): Unit = {
    scheduler executeXml <modify_order job_chain={o.jobChainPath.string} order={o.id.string} suspended="false"/>
  }
}

private object JS864IT {
  private val jobChainPath = JobChainPath("/test")
  private val aState = OrderState("A")
  private val bState = OrderState("B")
  private val cState = OrderState("C")
  private val processAction = "process"
  private val nextStateAction = "next_state"

  private def newOrderElem(orderId: OrderId) =
    <order job_chain={jobChainPath.string} id={orderId.string} suspended="true">
      <params>
        <param name="count" value="0"/>
      </params>
    </order>
}
