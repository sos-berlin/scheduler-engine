package com.sos.scheduler.engine.tests.jira.js864

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
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
    for (i ← 1 to 5)
      scheduler executeXml OrderCommand(NonpermanentJobChainPath orderKey OrderId(s"$i"), suspended = Some(true))
    // Test group 6 will add its order itself, so the order will have a clean initial state
  }

  addOrderTests(1, "All job chain node have action='process'", Nil) { orderKey ⇒
    nextOrderEvent(orderKey) shouldBe OrderStarted(orderKey)
    expectOrderStepStartedEvent(orderKey, AState)
    expectOrderStepStartedEvent(orderKey, BState)
    expectOrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinished(orderKey, EndState)
  }

  addOrderTests(2, "Job chain node B has action='next_state'", List(BState → NextStateAction)) { orderKey ⇒
    nextOrderEvent(orderKey) shouldBe OrderStarted(orderKey)
    expectOrderStepStartedEvent(orderKey, AState)
    expectOrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinished(orderKey, EndState)
  }

  addOrderTests(3, "Job chain node A has action='next_state'", List(BState → ProcessAction, AState → NextStateAction)) { orderKey ⇒
    // Alle wartenden Auftrage wechseln zu B
    nextOrderEvent(orderKey) shouldBe OrderStarted(orderKey)
    expectOrderStepStartedEvent(orderKey, BState)
    expectOrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinished(orderKey, EndState)
  }

  addOrderTests(4, "Again, all job chain nodes have action='process'", List(AState → ProcessAction)) { orderKey ⇒
    nextOrderEvent(orderKey) shouldBe OrderStarted(orderKey)
    // AState nicht, weil next_state im vorangehenden Test den Auftrag schon weitergeschoben hat.
    expectOrderStepStartedEvent(orderKey, BState)
    expectOrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinished(orderKey, EndState)
  }

  addOrderTests(5, "All job chain nodes have action='next_state'", List(AState → NextStateAction, BState → NextStateAction, CState → NextStateAction)) {
    case PermanentOrderKey ⇒ // A permanent order does not issue an OrderFinished ...
    case orderKey ⇒ nextOrderEvent(orderKey) shouldBe OrderFinished(orderKey, EndState)
  }

  addOrderTests(6, "Again, all job chain nodes have action='process'", List(AState → ProcessAction, BState → ProcessAction, CState → ProcessAction),
      addOrderFor = Set(NonpermanentJobChainPath)) { orderKey ⇒
    nextOrderEvent(orderKey) shouldBe OrderStarted(orderKey)
    expectOrderStepStartedEvent(orderKey, AState)
    expectOrderStepStartedEvent(orderKey, BState)
    expectOrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinished(orderKey, EndState)
  }

  private def addOrderTests(index: Int, caption: String, nodeActions: List[(OrderState, String)], addOrderFor: Set[JobChainPath] = Set())(body: OrderKey ⇒ Unit): Unit =
    s"$index) $caption" - {
      for (orderKey ← List(NonpermanentJobChainPath orderKey OrderId(s"$index"), PermanentOrderKey)) {
        s"Order $orderKey" in {
          if (!addOrderFor(orderKey.jobChainPath)) {
            suspendOrder(orderKey)
          }
          for ((state, action) ← nodeActions) {
            scheduler executeXml <job_chain_node.modify job_chain={orderKey.jobChainPath.string} state={state.string} action={action}/>
          }
          if (addOrderFor(orderKey.jobChainPath)) {
            scheduler executeXml OrderCommand(orderKey)
          } else {
            resumeOrder(orderKey)
          }
          body(orderKey)
        }
      }
    }

  private def expectOrderStepStartedEvent(orderKey: OrderKey, orderState: OrderState): Unit = {
    val e = nextOrderEvent(orderKey).asInstanceOf[OrderStepStarted]
    assert(e.orderKey == orderKey && e.state == orderState)
  }
  private def nextOrderEvent(orderKey: OrderKey) =
    eventPipe.nextWithCondition { e: OrderEvent ⇒ e.orderKey == orderKey && isRelevantOrderEventClass(e.getClass) }

  private def isRelevantOrderEventClass(eventClass: Class[_ <: OrderEvent]) =
    List(classOf[OrderStarted], classOf[OrderFinished], classOf[OrderStepStarted]) exists { _ isAssignableFrom eventClass }

  private def suspendOrder(orderKey: OrderKey): Unit =
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(true))

  private def resumeOrder(orderKey: OrderKey): Unit =
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false), at = Some(ModifyOrderCommand.NowAt))
}

private object JS864IT {
  private val NonpermanentJobChainPath = JobChainPath("/test-nonpermanent")
  private val PermanentJobChainPath = JobChainPath("/test-permanent")
  private val PermanentOrderKey = PermanentJobChainPath orderKey "permanent"
  private val AState = OrderState("A")
  private val BState = OrderState("B")
  private val CState = OrderState("C")
  private val EndState = OrderState("end")
  private val ProcessAction = "process"
  private val NextStateAction = "next_state"
}
