package com.sos.scheduler.engine.tests.jira.js1836

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedEvent, FileBasedReplaced}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderAdded, OrderEvent, OrderFinished, OrderId, OrderNodeChanged, OrderNodeTransition, OrderRemoved, OrderResumed, OrderStarted, OrderStepEnded, OrderStepStarted, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1836IT extends FreeSpec with ScalaSchedulerTest
{
  "Change of workflow keeps a suspended order at its node" in {
    val jobChainPath = JobChainPath("/test")
    val orderKey = jobChainPath orderKey OrderId("TEST")
    val eventPipe = controller.newEventPipe()

    eventBus.awaiting[OrderSuspended.type](orderKey) {
      startOrder(orderKey)
    }
    assert(order(orderKey).nodeId == NodeId("SUSPENDED"))

    updateFoldersWith(jobChainPath) {
      testEnvironment.fileFromPath(jobChainPath).xml =
      <job_chain>
        <job_chain_node state="CHANGED-FIRST" job="test"/>
        <job_chain_node state="SUSPENDED" job="test" suspend="true"/>
        <job_chain_node state="CHANGED-LAST" job="test"/>
        <job_chain_node.end state="CHANGED-END"/>
      </job_chain>
    }
    assert(order(orderKey).nodeId == NodeId("SUSPENDED"))

    eventBus.awaiting[OrderRemoved.type](orderKey) {
      eventBus.awaiting[OrderFinished](orderKey) {
        scheduler.executeXml(ModifyOrderCommand(orderKey, suspended = Some(false)))
      }
    }

    scheduler.executeCallQueue()
    assert(eventPipe.queued[Event].collect { case o @ KeyedEvent(_, _: OrderEvent | _: FileBasedEvent) => o } ==
      Vector(
        orderKey <-: OrderAdded(NodeId("FIRST")),
        orderKey <-: OrderStarted,
        orderKey <-: OrderStepStarted(NodeId("FIRST"), TaskId.First + 0),
        orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
        orderKey <-: OrderSuspended,
        orderKey <-: OrderNodeChanged(NodeId("SUSPENDED"), NodeId("FIRST")),
        jobChainPath <-: FileBasedReplaced,
        jobChainPath <-: FileBasedActivated,
        orderKey <-: OrderResumed,
        orderKey <-: OrderStepStarted(NodeId("SUSPENDED"), TaskId.First + 1),
        orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
        orderKey <-: OrderNodeChanged(NodeId("CHANGED-LAST"), fromNodeId = NodeId("SUSPENDED")) ,
        orderKey <-: OrderStepStarted(NodeId("CHANGED-LAST"), TaskId.First + 2),
        orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
        orderKey <-: OrderNodeChanged(NodeId("CHANGED-END"), fromNodeId = NodeId("CHANGED-LAST")),
        orderKey <-: OrderFinished(NodeId("CHANGED-END")),
        orderKey <-: OrderRemoved))
  }
}
