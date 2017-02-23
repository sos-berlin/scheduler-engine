package com.sos.scheduler.engine.tests.jira.js864

import com.sos.jobscheduler.common.scalautil.Closers.implicits._
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.jobscheduler.data.order.OrderId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
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
    nextOrderEvent(orderKey) shouldBe OrderStarted
    expectOrderStepStartedEvent(orderKey, ANodeId)
    expectOrderStepStartedEvent(orderKey, BNodeId)
    expectOrderStepStartedEvent(orderKey, CNodeId)
    nextOrderEvent(orderKey) shouldBe OrderFinished(EndNodeId)
  }

  addOrderTests(2, "Job chain node B has action='next_state'", List(BNodeId → NextStateAction)) { orderKey ⇒
    nextOrderEvent(orderKey) shouldBe OrderStarted
    expectOrderStepStartedEvent(orderKey, ANodeId)
    expectOrderStepStartedEvent(orderKey, CNodeId)
    nextOrderEvent(orderKey) shouldBe OrderFinished(EndNodeId)
  }

  addOrderTests(3, "Job chain node A has action='next_state'", List(BNodeId → ProcessAction, ANodeId → NextStateAction)) { orderKey ⇒
    // Alle wartenden Auftrage wechseln zu B
    nextOrderEvent(orderKey) shouldBe OrderStarted
    expectOrderStepStartedEvent(orderKey, BNodeId)
    expectOrderStepStartedEvent(orderKey, CNodeId)
    nextOrderEvent(orderKey) shouldBe OrderFinished(EndNodeId)
  }

  addOrderTests(4, "Again, all job chain nodes have action='process'", List(ANodeId → ProcessAction)) { orderKey ⇒
    nextOrderEvent(orderKey) shouldBe OrderStarted
    // ANodeId nicht, weil next_state im vorangehenden Test den Auftrag schon weitergeschoben hat.
    expectOrderStepStartedEvent(orderKey, BNodeId)
    expectOrderStepStartedEvent(orderKey, CNodeId)
    nextOrderEvent(orderKey) shouldBe OrderFinished(EndNodeId)
  }

  addOrderTests(5, "All job chain nodes have action='next_state'", List(ANodeId → NextStateAction, BNodeId → NextStateAction, CNodeId → NextStateAction)) {
    case PermanentOrderKey ⇒ // A permanent order does not issue an OrderFinished ...
    case orderKey ⇒ nextOrderEvent(orderKey) shouldBe OrderFinished(EndNodeId)
  }

  addOrderTests(6, "Again, all job chain nodes have action='process'", List(ANodeId → ProcessAction, BNodeId → ProcessAction, CNodeId → ProcessAction),
      addOrderFor = Set(NonpermanentJobChainPath)) { orderKey ⇒
    nextOrderEvent(orderKey) shouldBe OrderStarted
    expectOrderStepStartedEvent(orderKey, ANodeId)
    expectOrderStepStartedEvent(orderKey, BNodeId)
    expectOrderStepStartedEvent(orderKey, CNodeId)
    nextOrderEvent(orderKey) shouldBe OrderFinished(EndNodeId)
  }

  private def addOrderTests(index: Int, caption: String, nodeActions: List[(NodeId, String)], addOrderFor: Set[JobChainPath] = Set())(body: OrderKey ⇒ Unit): Unit =
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

  private def expectOrderStepStartedEvent(orderKey: OrderKey, orderState: NodeId): Unit = {
    val e = nextOrderEvent(orderKey).asInstanceOf[OrderStepStarted]
    assert(e.nodeId == orderState)
  }
  private def nextOrderEvent(orderKey: OrderKey): OrderEvent =
    eventPipe.nextWhen[OrderEvent] {
      case KeyedEvent(`orderKey`, OrderStarted | _: OrderStepStarted | _: OrderFinished) ⇒ true
      case _ ⇒ false
    }.event

  private def suspendOrder(orderKey: OrderKey): Unit =
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(true))

  private def resumeOrder(orderKey: OrderKey): Unit =
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false), at = Some(ModifyOrderCommand.NowAt))
}

private object JS864IT {
  private val NonpermanentJobChainPath = JobChainPath("/test-nonpermanent")
  private val PermanentJobChainPath = JobChainPath("/test-permanent")
  private val PermanentOrderKey = PermanentJobChainPath orderKey "permanent"
  private val ANodeId = NodeId("A")
  private val BNodeId = NodeId("B")
  private val CNodeId = NodeId("C")
  private val EndNodeId = NodeId("end")
  private val ProcessAction = "process"
  private val NextStateAction = "next_state"
}
