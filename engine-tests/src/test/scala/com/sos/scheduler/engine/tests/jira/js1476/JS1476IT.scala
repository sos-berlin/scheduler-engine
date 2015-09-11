package com.sos.scheduler.engine.tests.jira.js1476

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand.NowAt
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1476.JS1476IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1476 Completely skipping nested jobchains
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1476IT extends FreeSpec with ScalaSchedulerTest {

  private val superOrderKeys = for (i ← Iterator.from(1)) yield SuperJobChainPath orderKey s"$i"

  "Resuming order staying in completely skipped nested last job chain" in {
    val superOrderKey = superOrderKeys.next()
    val cOrderKey = JobChainPath("/test-nested-c") orderKey superOrderKey.id
    val stoppedState = OrderState("NESTED-C-2")
    scheduler executeXml <job_chain_node.modify job_chain={CJobChainPath.string} state={stoppedState.string} action="stop"/>
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](cOrderKey) {
      eventBus.awaitingKeyedEvent[OrderStateChangedEvent](cOrderKey) {
        scheduler executeXml OrderCommand(superOrderKey)
      } .state shouldEqual stoppedState
      scheduler executeXml <job_chain_node.modify job_chain={CJobChainPath.string} state="NESTED-C-3" action="next_state"/> // Last node
      scheduler executeXml <job_chain_node.modify job_chain={CJobChainPath.string} state="NESTED-C-2" action="next_state"/> // Then current node
    }
  }

  "Continuing order when next and last nested job chain are completely skipped" in {
    setSkippingNodes(BOrderStates ++ COrderStates)
    val superOrderKey = superOrderKeys.next()
    val aOrderKey = AJobChainPath orderKey superOrderKey.id
    val stoppedState = OrderState("NESTED-A-2")
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedState.string} action="stop"/>
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](aOrderKey) {
      eventBus.awaitingKeyedEvent[OrderStateChangedEvent](aOrderKey) {
        scheduler executeXml OrderCommand(superOrderKey)
      } .state shouldEqual stoppedState
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state="NESTED-A-2" action="process"/>
    }
  }

  "Resuming suspended order when next nested job chain is completely skipped" in {
    // Jobchain test-nested-b is completely skipping
    val superOrderKey = superOrderKeys.next()
    val aOrderKey = AJobChainPath orderKey superOrderKey.id
    val stoppedState = OrderState("NESTED-A-2")
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedState.string} action="stop"/>
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](aOrderKey) {
      eventBus.awaitingKeyedEvent[OrderStateChangedEvent](aOrderKey) {
        scheduler executeXml OrderCommand(superOrderKey)
      } .state shouldEqual stoppedState
      scheduler executeXml OrderCommand(aOrderKey, suspended = Some(true))
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state="NESTED-A-2" action="next_state"/>
      scheduler executeXml OrderCommand(aOrderKey, suspended = Some(false))
    }
  }

  "Adding order when all nested job chains are completely skipping" in {
    setSkippingNodes(AOrderStates ++ BOrderStates ++ COrderStates)
    // All nested jobchains are completely skipped
    val superOrderKey = superOrderKeys.next()
    interceptSchedulerError(MessageCode("SCHEDULER-438")) {  // "Invalid Job_chain_node for add_order() with state='END'"
      scheduler executeXml OrderCommand(superOrderKey)
    }
  }

  "Adding order when only the first nested job chain is completely skipping" in {
    setSkippingNodes(AOrderStates)
    val superOrderKey = superOrderKeys.next()
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](cOrderKey) {
      scheduler executeXml OrderCommand(superOrderKey)
    }
  }

  "Adding order when only the middle nested job chain is completely skipping" in {
    setSkippingNodes(BOrderStates)
    val superOrderKey = superOrderKeys.next()
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](cOrderKey) {
      scheduler executeXml OrderCommand(superOrderKey)
    }
  }

  "Adding and repeating a permanent order when first nested job chain is completely skipping" in {
    setSkippingNodes(AOrderStates ++ BOrderStates)
    val superOrderKey = superOrderKeys.next()
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](cOrderKey) {
      writeConfigurationFile(superOrderKey, <order/>)
    }
    val visibleOrdersIds = (scheduler executeXml <show_job_chain job_chain={CJobChainPath.string}/>).elem \
      "answer" \ "job_chain" \ "job_chain_node" \ "order_queue" \ "order" \ "@order" map { o ⇒ OrderId(o.text) }
    assert(visibleOrdersIds contains superOrderKey.id)
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](cOrderKey) {
      scheduler executeXml ModifyOrderCommand(CJobChainPath orderKey superOrderKey.id, at = Some(NowAt))
    }
    deleteConfigurationFile(superOrderKey)
  }

  "Starting and repeating a permanent order when first nested job chain is completely skipping" in {
    val superOrderKey = superOrderKeys.next()
    writeConfigurationFile(superOrderKey, <order><run_time><at at="1999-01-01"/></run_time></order>)
    setSkippingNodes(AOrderStates ++ BOrderStates)
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](cOrderKey) {
      scheduler executeXml ModifyOrderCommand(CJobChainPath orderKey superOrderKey.id, at = Some(NowAt))
    }
    val visibleOrdersIds = (scheduler executeXml <show_job_chain job_chain={CJobChainPath.string}/>).elem \
      "answer" \ "job_chain" \ "job_chain_node" \ "order_queue" \ "order" \ "@order" map { o ⇒ OrderId(o.text) }
    assert(visibleOrdersIds contains superOrderKey.id)
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](cOrderKey) {
      scheduler executeXml ModifyOrderCommand(CJobChainPath orderKey superOrderKey.id, at = Some(NowAt))
    }
    deleteConfigurationFile(superOrderKey)
  }

  // "Order.reset" in { see JS631IT }

  private def setSkippingNodes(states: Set[OrderState]): Unit = {
    def set(jobChainPath: JobChainPath, state: OrderState): Unit = {
      val action = if (states(state)) "next_state" else "process"
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state={state.string} action={action}/>
    }
    for (state ← AOrderStates) set(AJobChainPath, state)
    for (state ← BOrderStates) set(BJobChainPath, state)
    for (state ← COrderStates) set(CJobChainPath, state)
  }
}

private object JS1476IT {
  private val SuperJobChainPath = JobChainPath("/test-super")
  private val AJobChainPath = JobChainPath("/test-nested-a")
  private val BJobChainPath = JobChainPath("/test-nested-b")
  private val CJobChainPath = JobChainPath("/test-nested-c")
  private val AOrderStates = Set(OrderState("NESTED-A-1"), OrderState("NESTED-A-2"), OrderState("NESTED-A-3"))
  private val BOrderStates = Set(OrderState("NESTED-B-1"), OrderState("NESTED-B-2"), OrderState("NESTED-B-3"))
  private val COrderStates = Set(OrderState("NESTED-C-1"), OrderState("NESTED-C-2"), OrderState("NESTED-C-3"))
}
