package com.sos.scheduler.engine.tests.jira.js1662

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey, OrderState, OrderStateChangedEvent}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1662.JS1662IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
  * JS-1662 Modified end_state should be reset when order finishes or is reset.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1662IT extends FreeSpec with ScalaSchedulerTest {

  "Unchanged end NodeId" in {
    expectOrdersFinishAt(ConfiguredEndNodeIds) { orderKey ⇒
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
    }
    expectOrdersFinishAt(ConfiguredEndNodeIds) { orderKey ⇒
      scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Changed end NodeId is reset when order has been carried out" in {
    scheduler executeXml <job_chain_node.modify job_chain="/test" state="200" action="stop"/>
    eventBus.awaitingEvent[OrderStateChangedEvent](e ⇒ e.orderKey == TestOrderKeys(0) && e.state == OrderState("200")) {
      eventBus.awaitingEvent[OrderStateChangedEvent](e ⇒ e.orderKey == TestOrderKeys(1) && e.state == OrderState("200")) {
        for (orderKey ← TestOrderKeys) {
          scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
        }
      }
    }
    // In the middle of the chain, we change the end NodeIds of the orders and resume
    expectOrdersFinishAt(List(ChangedEndNodeId, ChangedEndNodeId)) { orderKey ⇒
      scheduler executeXml ModifyOrderCommand(orderKey, endState = Some(ChangedEndNodeId), suspended = Some(false))
      scheduler executeXml <job_chain_node.modify job_chain="/test" state="200" action="process"/>
    }
    // Second run of same orders, to check its end NodeIds
    expectOrdersFinishAt(ConfiguredEndNodeIds) {
      scheduler executeXml ModifyOrderCommand(_, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Changed end NodeId when order is reset by command" in {
    expectOrdersFinishAt(ConfiguredEndNodeIds) { orderKey ⇒
      scheduler executeXml ModifyOrderCommand(orderKey, endState = Some(ChangedEndNodeId))
      scheduler executeXml ModifyOrderCommand(orderKey, action = Some(ModifyOrderCommand.Action.reset))
      scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  private def expectOrdersFinishAt(nodeIds: Seq[OrderState])(body: OrderKey ⇒ Unit): Unit = {
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](TestOrderKeys(1)) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](TestOrderKeys(0)) {
        for (o ← TestOrderKeys) {
          body(o)
        }
      } .state shouldEqual nodeIds(0)
    } .state shouldEqual nodeIds(1)
  }
}

private object JS1662IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestOrderKeys = List("0", "1") map TestJobChainPath.orderKey
  private val ConfiguredEndNodeIds = List(OrderState("END"), OrderState("300"))
  private val ChangedEndNodeId = OrderState("200")
}
