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
    for (i ← 1 to 5)
      scheduler executeXml OrderCommand(NonpermanentJobChainPath orderKey OrderId(s"$i"), suspended = Some(true))
    // Test group 6 will add its order itself, so the order will have a clean initial state
  }

  addOrderTests(1, "All job chain node have action='process'", Nil) { orderKey ⇒
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, AState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, BState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  addOrderTests(2, "Job chain node B has action='next_state'", List(BState → NextStateAction)) { orderKey ⇒
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, AState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  addOrderTests(3, "Job chain node A has action='next_state'", List(BState → ProcessAction, AState → NextStateAction)) { orderKey ⇒
    // Alle wartenden Auftrage wechseln zu B
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, BState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  addOrderTests(4, "Again, all job chain nodes have action='process'", List(AState → ProcessAction)) { orderKey ⇒
    resumeOrder(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    // AState nicht, weil next_state im vorangehenden Test den Auftrag schon weitergeschoben hat.
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, BState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  addOrderTests(5, "All job chain nodes have action='next_state'", List(AState → NextStateAction, BState → NextStateAction, CState → NextStateAction)) { orderKey ⇒
    resumeOrder(orderKey)
    orderKey match {
      case PermanentOrderKey ⇒ // A permanent order does not issue an OrderFinishedEvent ...
      case _ ⇒ nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
    }
  }

  addOrderTests(6, "Again, all job chain nodes have action='process'", List(AState → ProcessAction, BState → ProcessAction, CState → ProcessAction)) { orderKey ⇒
    if (orderKey.jobChainPath == NonpermanentJobChainPath) {
      // Add new order 6
      scheduler executeXml OrderCommand(orderKey)
    }
    nextOrderEvent(orderKey) shouldBe OrderTouchedEvent(orderKey)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, AState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, BState)
    nextOrderEvent(orderKey) shouldBe OrderStepStartedEvent(orderKey, CState)
    nextOrderEvent(orderKey) shouldBe OrderFinishedEvent(orderKey)
  }

  private def addOrderTests(index: Int, caption: String, nodeActions: List[(OrderState, String)])(body: OrderKey ⇒ Unit): Unit =
    s"$index) $caption" - {
      for (orderKey ← List(NonpermanentJobChainPath orderKey OrderId(s"$index"), PermanentOrderKey)) {
        s"Order $orderKey" in {
          for ((state, action) ← nodeActions)
            scheduler executeXml <job_chain_node.modify job_chain={orderKey.jobChainPath.string} state={state.string} action={action}/>
          body(orderKey)
        }
      }
    }

  private def nextOrderEvent(orderKey: OrderKey) =
    eventPipe.nextWithCondition { e: OrderEvent ⇒ e.orderKey == orderKey && isRelevantOrderEventClass(e.getClass) }

  private def isRelevantOrderEventClass(eventClass: Class[_ <: OrderEvent]) =
    List(classOf[OrderTouchedEvent], classOf[OrderFinishedEvent], classOf[OrderStepStartedEvent]) exists { _ isAssignableFrom eventClass }

  private def resumeOrder(o: OrderKey): Unit =
    scheduler executeXml <modify_order job_chain={o.jobChainPath.string} order={o.id.string} suspended="false" at="now"/>
}

private object JS864IT {
  private val NonpermanentJobChainPath = JobChainPath("/test-nonpermanent")
  private val PermanentJobChainPath = JobChainPath("/test-permanent")
  private val PermanentOrderKey = PermanentJobChainPath orderKey "permanent"
  private val AState = OrderState("A")
  private val BState = OrderState("B")
  private val CState = OrderState("C")
  private val ProcessAction = "process"
  private val NextStateAction = "next_state"
}
