package com.sos.scheduler.engine.tests.jira.js864

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js864.JS864IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS864IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val eventPipe = controller.newEventPipe().closeWithCloser

  override def onBeforeSchedulerActivation() = eventPipe

  override def onSchedulerActivated() = {
    for (i ← 0 to 5)
      scheduler executeXml OrderCommand(TestJobChainPath orderKey OrderId(i.toString), suspended = Some(true), parameters = Map("count" → "0"))
  }

  "0) All job chain node have action='process'" in {
    val orderKey = TestJobChainPath orderKey OrderId("0")
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, AState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, BState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  "1) Job chain node B has action='next_state'" in {
    modifyNode(BState, NextStateAction)
    val orderKey = TestJobChainPath orderKey OrderId("1")
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, AState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  "2) Job chain node A has action='next_state'" in {
    modifyNode(BState, ProcessAction)
    modifyNode(AState, NextStateAction)   // Alle wartenden Auftrage wechseln zu B
    val orderKey = TestJobChainPath orderKey OrderId("2")
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, BState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  "3) Again, all job chain nodes have action='process'" in {
    modifyNode(AState, ProcessAction)
    val orderKey = TestJobChainPath orderKey OrderId("3")
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    // aState nicht, weil next_state im vorangehenden Test den Auftrag schon weitergeschoben hat.
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, BState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  "4) All job chain nodes have action='next_state'" in {
    modifyNode(AState, NextStateAction)
    modifyNode(BState, NextStateAction)
    modifyNode(CState, NextStateAction)
    val orderKey = TestJobChainPath orderKey OrderId("4")
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  "5) Again, no job chain node having action='next_state'" in {
    modifyNode(BState, "process")
    val orderKey = TestJobChainPath orderKey OrderId("5")
    resumeOrder(orderKey)
    //intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition(2.s) { e: OrderTouchedEvent ⇒ e.getKey == jobChainPath.orderKey(orderId) } }
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  private def nextOrderEvent(orderKey: OrderKey) =
    eventPipe.nextWithCondition { e: OrderEvent ⇒ e.orderKey == orderKey && isRelevantOrderEventClass(e.getClass) }

  private def isRelevantOrderEventClass(eventClass: Class[_ <: OrderEvent]) =
    List(classOf[OrderTouchedEvent], classOf[OrderFinishedEvent], classOf[OrderStepStartedEvent]) exists { _ isAssignableFrom eventClass }

  private def modifyNode(state: OrderState, action: String): Unit =
    scheduler executeXml <job_chain_node.modify job_chain={TestJobChainPath.string} state={state.string} action={action}/>

  private def resumeOrder(o: OrderKey): Unit =
    scheduler executeXml <modify_order job_chain={o.jobChainPath.string} order={o.id.string} suspended="false"/>
}

private object JS864IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val AState = OrderState("A")
  private val BState = OrderState("B")
  private val CState = OrderState("C")
  private val ProcessAction = "process"
  private val NextStateAction = "next_state"
}
