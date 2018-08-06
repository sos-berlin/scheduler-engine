package com.sos.scheduler.engine.tests.jira.js1476

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
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
    val stoppedNodeId = NodeId("NESTED-C-2")
    scheduler executeXml <job_chain_node.modify job_chain={CJobChainPath.string} state={stoppedNodeId.string} action="stop"/>
    eventBus.awaiting[OrderFinished](cOrderKey) {
      eventBus.awaiting[OrderNodeChanged](cOrderKey) {
        scheduler executeXml OrderCommand(superOrderKey)
      } .nodeId shouldEqual stoppedNodeId
      scheduler executeXml <job_chain_node.modify job_chain={CJobChainPath.string} state="NESTED-C-3" action="next_state"/> // Last node
      scheduler executeXml <job_chain_node.modify job_chain={CJobChainPath.string} state="NESTED-C-2" action="next_state"/> // Then current node
    }
  }

  "Continuing order when next and last nested job chain are completely skipped" in {
    setSkippingNodes(BOrderNodeIds ++ COrderNodeIds)
    val superOrderKey = superOrderKeys.next()
    val aOrderKey = AJobChainPath orderKey superOrderKey.id
    val stoppedNodeId = NodeId("NESTED-A-2")
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedNodeId.string} action="stop"/>
    eventBus.awaiting[OrderFinished](aOrderKey) {
      eventBus.awaiting[OrderNodeChanged](aOrderKey) {
        scheduler executeXml OrderCommand(superOrderKey)
      } .nodeId shouldEqual stoppedNodeId
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state="NESTED-A-2" action="process"/>
    }
  }

  "Resuming suspended order when next nested job chain is completely skipped" in {
    // Jobchain test-nested-b is completely skipping
    val superOrderKey = superOrderKeys.next()
    val aOrderKey = AJobChainPath orderKey superOrderKey.id
    val stoppedNodeId = NodeId("NESTED-A-2")
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedNodeId.string} action="stop"/>
    eventBus.awaiting[OrderFinished](aOrderKey) {
      eventBus.awaiting[OrderNodeChanged](aOrderKey) {
        scheduler executeXml OrderCommand(superOrderKey)
      } .nodeId shouldEqual stoppedNodeId
      scheduler executeXml ModifyOrderCommand(aOrderKey, suspended = Some(true))
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state="NESTED-A-2" action="next_state"/>
      scheduler executeXml ModifyOrderCommand(aOrderKey, suspended = Some(false))
    }
  }

  "Continuing order at stopped node when all nested job chains are completely skipped (JS-1772)" in {
    val superOrderKey = superOrderKeys.next()
    val aOrderKey = AJobChainPath orderKey superOrderKey.id
    val stoppedNodeId = NodeId("NESTED-A-2")
    setSkippingNodes(AOrderNodeIds ++ BOrderNodeIds ++ COrderNodeIds)
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state="NESTED-A-1" action="process"/>
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedNodeId.string} action="stop"/>
    eventBus.awaiting[OrderFinished](aOrderKey) {
      eventBus.awaiting[OrderNodeChanged](aOrderKey) {
        scheduler executeXml OrderCommand(superOrderKey)
      } .nodeId shouldEqual stoppedNodeId
      scheduler executeXml ModifyOrderCommand(aOrderKey, suspended = Some(true))
      // Skip the last node, so each node of each nested job chain is skipped
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedNodeId.string} action="next_state"/>
      scheduler executeXml ModifyOrderCommand(aOrderKey, suspended = Some(false))
    }
  }

  "Continuing order at stopped node when all nested job chains are completely skipped (JS-1772) - permanent" in {
    val superOrderKey = superOrderKeys.next()
    val aOrderKey = AJobChainPath orderKey superOrderKey.id
    val stoppedNodeId = NodeId("NESTED-A-2")
    setSkippingNodes(AOrderNodeIds ++ BOrderNodeIds ++ COrderNodeIds)
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state="NESTED-A-1" action="process"/>
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedNodeId.string} action="stop"/>
    eventBus.awaiting[OrderFinished](aOrderKey) {
      eventBus.awaiting[OrderNodeChanged](aOrderKey) {
        writeConfigurationFile(superOrderKey, <order/>)
      } .nodeId shouldEqual stoppedNodeId
      scheduler executeXml ModifyOrderCommand(aOrderKey, suspended = Some(true))
      // Skip the last node, so each node of each nested job chain is skipped
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={stoppedNodeId.string} action="next_state"/>
      scheduler executeXml ModifyOrderCommand(aOrderKey, suspended = Some(false))
    }

    // Second run after order.xml has been changed
    scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state="NESTED-A-1" action="process"/>
    eventBus.awaiting[OrderFinished](aOrderKey) {
      writeConfigurationFile(superOrderKey, <order  />)
    }
  }

  "Adding order when all nested job chains are completely skipping" in {
    setSkippingNodes(AOrderNodeIds ++ BOrderNodeIds ++ COrderNodeIds)
    // All nested jobchains are completely skipped
    val superOrderKey = superOrderKeys.next()
    interceptSchedulerError(MessageCode("SCHEDULER-438")) {  // "Invalid Job_chain_node for add_order() with nodeId='END'"
      scheduler executeXml OrderCommand(superOrderKey)
    }
  }

  "Adding order when only the first nested job chain is completely skipping" in {
    setSkippingNodes(AOrderNodeIds)
    val superOrderKey = superOrderKeys.next()
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    eventBus.awaiting[OrderFinished](cOrderKey) {
      scheduler executeXml OrderCommand(superOrderKey)
    }
  }

  "Adding order when only the middle nested job chain is completely skipping" in {
    setSkippingNodes(BOrderNodeIds)
    val superOrderKey = superOrderKeys.next()
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    eventBus.awaiting[OrderFinished](cOrderKey) {
      scheduler executeXml OrderCommand(superOrderKey)
    }
  }

  "Adding and repeating a permanent order when first nested job chain is completely skipping" in {
    setSkippingNodes(AOrderNodeIds ++ BOrderNodeIds)
    val superOrderKey = superOrderKeys.next()
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    eventBus.awaiting[OrderFinished](cOrderKey) {
      writeConfigurationFile(superOrderKey, <order/>)
    }
    requireOrderIsVisible(cOrderKey)
    eventBus.awaiting[OrderFinished](cOrderKey) {
      scheduler executeXml ModifyOrderCommand(CJobChainPath orderKey superOrderKey.id, at = Some(NowAt))
    }
    requireOrderIsVisible(cOrderKey)
    deleteConfigurationFile(superOrderKey)
  }

  "Starting and repeating a permanent order when first nested job chain is completely skipping" in {
    setSkippingNodes(Set())
    val superOrderKey = superOrderKeys.next()
    writeConfigurationFile(superOrderKey, <order><run_time><at at="1999-01-01"/></run_time></order>)
    requireOrderIsVisible(AJobChainPath orderKey superOrderKey.id)
    setSkippingNodes(AOrderNodeIds ++ BOrderNodeIds)
    val cOrderKey = CJobChainPath orderKey superOrderKey.id
    requireOrderIsVisible(cOrderKey)
    eventBus.awaiting[OrderFinished](cOrderKey) {
      scheduler executeXml ModifyOrderCommand(CJobChainPath orderKey superOrderKey.id, at = Some(NowAt))
    }
    requireOrderIsVisible(cOrderKey)
    eventBus.awaiting[OrderFinished](cOrderKey) {
      scheduler executeXml ModifyOrderCommand(CJobChainPath orderKey superOrderKey.id, at = Some(NowAt))
    }
    requireOrderIsVisible(cOrderKey)
    deleteConfigurationFile(superOrderKey)
  }

  // "Order.reset" in { see JS631IT }

  private def setSkippingNodes(states: Set[NodeId]): Unit = {
    def set(jobChainPath: JobChainPath, nodeId: NodeId): Unit = {
      val action = if (states(nodeId)) "next_state" else "process"
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state={nodeId.string} action={action}/>
    }
    for (nodeId ← AOrderNodeIds) set(AJobChainPath, nodeId)
    for (nodeId ← BOrderNodeIds) set(BJobChainPath, nodeId)
    for (nodeId ← COrderNodeIds) set(CJobChainPath, nodeId)
  }

  def requireOrderIsVisible(orderKey: OrderKey): Unit = {
    val visibleOrdersIds = (scheduler executeXml <show_job_chain job_chain={orderKey.jobChainPath.string}/>).elem \
      "answer" \ "job_chain" \ "job_chain_node" \ "order_queue" \ "order" \ "@order" map { o ⇒ OrderId(o.text) }
    assert(visibleOrdersIds contains orderKey.id)
  }
}

private object JS1476IT {
  private val SuperJobChainPath = JobChainPath("/test-super")
  private val AJobChainPath = JobChainPath("/test-nested-a")
  private val BJobChainPath = JobChainPath("/test-nested-b")
  private val CJobChainPath = JobChainPath("/test-nested-c")
  private val AOrderNodeIds = Set(NodeId("NESTED-A-1"), NodeId("NESTED-A-2"), NodeId("NESTED-A-3"))
  private val BOrderNodeIds = Set(NodeId("NESTED-B-1"), NodeId("NESTED-B-2"), NodeId("NESTED-B-3"))
  private val COrderNodeIds = Set(NodeId("NESTED-C-1"), NodeId("NESTED-C-2"), NodeId("NESTED-C-3"))
}
