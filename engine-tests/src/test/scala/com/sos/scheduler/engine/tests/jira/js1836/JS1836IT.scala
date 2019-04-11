package com.sos.scheduler.engine.tests.jira.js1836

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedEvent, FileBasedReplaced}
import com.sos.scheduler.engine.data.job.{ReturnCode, TaskId}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderAdded, OrderEvent, OrderFinished, OrderId, OrderNodeChanged, OrderNodeTransition, OrderRemoved, OrderResumed, OrderStarted, OrderStepEnded, OrderStepStarted, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
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
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    ignoreError = Set(
      MessageCode("SCHEDULER-280"),
      MessageCode("SCHEDULER-149")))  // Change of first NodeId fails

  "Change of workflow keeps a suspended order at its node" - {
    "suspend=true" in {
      val jobChainPath = JobChainPath("/suspend")
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

      // The order keeps its position
      assert(order(orderKey).nodeId == NodeId("SUSPENDED"), s"(Order ${orderKey.id})")

      eventBus.awaiting[OrderRemoved.type](orderKey) {
        eventBus.awaiting[OrderFinished](orderKey) {
          scheduler.executeXml(ModifyOrderCommand(orderKey, suspended = Some(false)))
        }
      }

      scheduler.executeCallQueue()
      assert(eventPipe.queued[Event].collect {
        case o @ (KeyedEvent(`jobChainPath`, _: FileBasedEvent) | KeyedEvent(`orderKey`, _: OrderEvent)) => o
      } ==
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

    "suspend=true, permanent order" in {
      val jobChainPath = JobChainPath("/suspend-permanent")
      val orderKey = jobChainPath orderKey OrderId("PERMANENT")
      val eventPipe = controller.newEventPipe()

      eventBus.awaiting[OrderSuspended.type](orderKey) {
        scheduler.executeXml(ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt)))
      }

      assert(order(orderKey).nodeId == NodeId("SUSPENDED"))

      updateFoldersWith(jobChainPath) {
        testEnvironment.fileFromPath(jobChainPath).xml =
          <job_chain>
            <job_chain_node state="FIRST" job="test"/>
            <job_chain_node state="SUSPENDED" job="test" suspend="true"/>
            <job_chain_node state="CHANGED-LAST" job="test"/>
            <job_chain_node.end state="CHANGED-END"/>
          </job_chain>
      }

      // The running order keeps its position
      assert(order(orderKey).nodeId == NodeId("SUSPENDED"), s"(Order ${orderKey.id})")

      eventBus.awaiting[OrderFinished](orderKey) {
        scheduler.executeXml(ModifyOrderCommand(orderKey, suspended = Some(false)))
      }

      scheduler.executeCallQueue()
      assert(eventPipe.queued[Event].collect {
        case o @ (KeyedEvent(`jobChainPath`, _: FileBasedEvent) | KeyedEvent(`orderKey`, _: OrderEvent)) => o
      } ==
        Vector(
          orderKey <-: OrderStarted,
          orderKey <-: OrderStepStarted(NodeId("FIRST"), TaskId.First + 3),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
          orderKey <-: OrderSuspended,
          orderKey <-: OrderNodeChanged(NodeId("SUSPENDED"), NodeId("FIRST")),
          jobChainPath <-: FileBasedReplaced,
          jobChainPath <-: FileBasedActivated,
          orderKey <-: OrderResumed,
          orderKey <-: OrderStepStarted(NodeId("SUSPENDED"), TaskId.First + 4),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
          orderKey <-: OrderNodeChanged(NodeId("CHANGED-LAST"), fromNodeId = NodeId("SUSPENDED")) ,
          orderKey <-: OrderStepStarted(NodeId("CHANGED-LAST"), TaskId.First + 5),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
          orderKey <-: OrderNodeChanged(NodeId("CHANGED-END"), fromNodeId = NodeId("CHANGED-LAST")),
          orderKey <-: OrderFinished(NodeId("CHANGED-END")),
          orderKey <-: OrderNodeChanged(NodeId("FIRST"), fromNodeId = NodeId("CHANGED-END"))))
    }

    "FAILS: suspend=true, permanent order, first NodeId changed (see JS-1281)" in {
      val jobChainPath = JobChainPath("/suspend-permanent-2")
      val orderKey = jobChainPath orderKey OrderId("PERMANENT-CHANGED-FIRST")
      val eventPipe = controller.newEventPipe()

      eventBus.awaiting[OrderSuspended.type](orderKey) {
        scheduler.executeXml(ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt)))
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

      // The running order keeps its position
      assert(order(orderKey).nodeId == NodeId("SUSPENDED"), s"(Order ${orderKey.id})")

      eventBus.awaiting[OrderFinished](orderKey) {
        scheduler.executeXml(ModifyOrderCommand(orderKey, suspended = Some(false)))
      }
      assert(order(orderKey).nodeId == NodeId("CHANGED-END"))  // THIS IS WRONG. MUST BE "CHANGED-FIRST"
      scheduler.executeCallQueue()
      assert(eventPipe.queued[Event].collect {
        case o @ (KeyedEvent(`jobChainPath`, _: FileBasedEvent) | KeyedEvent(`orderKey`, _: OrderEvent)) => o
      } ==
        Vector(
          orderKey <-: OrderStarted,
          orderKey <-: OrderStepStarted(NodeId("FIRST"), TaskId.First + 6),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
          orderKey <-: OrderSuspended,
          orderKey <-: OrderNodeChanged(NodeId("SUSPENDED"), NodeId("FIRST")),
          jobChainPath <-: FileBasedReplaced,
          jobChainPath <-: FileBasedActivated,
          orderKey <-: OrderResumed,
          orderKey <-: OrderStepStarted(NodeId("SUSPENDED"), TaskId.First + 7),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
          orderKey <-: OrderNodeChanged(NodeId("CHANGED-LAST"), fromNodeId = NodeId("SUSPENDED")) ,
          orderKey <-: OrderStepStarted(NodeId("CHANGED-LAST"), TaskId.First + 8),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
          orderKey <-: OrderNodeChanged(NodeId("CHANGED-END"), fromNodeId = NodeId("CHANGED-LAST")),
          orderKey <-: OrderFinished(NodeId("CHANGED-END"))))
          //Does not happen: orderKey <-: OrderNodeChanged(NodeId("CHANGED-FIRST"), fromNodeId = NodeId("CHANGED-END"))))
    }

    "on_error=suspend" in {
      val jobChainPath = JobChainPath("/on-error-suspend")
      val orderKey = jobChainPath orderKey OrderId("TEST")
      val eventPipe = controller.newEventPipe()

      eventBus.awaiting[OrderSuspended.type](orderKey) {
        startOrder(orderKey)
      }

      assert(order(orderKey).nodeId == NodeId("ERROR"))

      updateFoldersWith(jobChainPath) {
        testEnvironment.fileFromPath(jobChainPath).xml =
          <job_chain>
            <job_chain_node state="CHANGED-FIRST" job="test"/>
            <job_chain_node state="ERROR" job="error" on_error="suspend"/>
            <job_chain_node state="CHANGED-LAST" job="test"/>
            <job_chain_node.end state="CHANGED-END"/>
          </job_chain>
      }
      assert(order(orderKey).nodeId == NodeId("ERROR"), s"(Order ${orderKey.id})")

      eventBus.awaiting[OrderSuspended.type](orderKey) {
        scheduler.executeXml(ModifyOrderCommand(orderKey, suspended = Some(false)))
      }

      scheduler.executeCallQueue()
      assert(eventPipe.queued[Event].collect {
        case o @ (KeyedEvent(`jobChainPath`, _: FileBasedEvent) | KeyedEvent(`orderKey`, _: OrderEvent)) => o
      } ==
        Vector(
          orderKey <-: OrderAdded(NodeId("FIRST")),
          orderKey <-: OrderStarted,
          orderKey <-: OrderStepStarted(NodeId("FIRST"), TaskId.First + 9),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
          orderKey <-: OrderNodeChanged(NodeId("ERROR"), fromNodeId = NodeId("FIRST")) ,
          orderKey <-: OrderStepStarted(NodeId("ERROR"), TaskId.First + 10),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Error(ReturnCode(1))),
          orderKey <-: OrderSuspended,
          jobChainPath <-: FileBasedReplaced,
          jobChainPath <-: FileBasedActivated,
          orderKey <-: OrderResumed,
          orderKey <-: OrderStepStarted(NodeId("ERROR"), TaskId.First + 11),
          orderKey <-: OrderStepEnded(OrderNodeTransition.Error(ReturnCode(1))),
          orderKey <-: OrderSuspended))
    }
  }
}
