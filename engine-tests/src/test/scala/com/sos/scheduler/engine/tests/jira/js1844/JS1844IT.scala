package com.sos.scheduler.engine.tests.jira.js1844

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1844.JS1844IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1844IT extends FreeSpec with ScalaSchedulerTest
{
  private val notSkipping = OrderCommand(jobChainPath orderKey "NOT-SKIPPED", nodeId = Some(SkippedNodeId), endNodeId = Some(SkippedNodeId))

  "Order's start node and end node are the same node" in {
    // No node skipping
    val result = runOrder(notSkipping)
    assert(result.nodeId == SkippedNodeId && result.variables("LAST-NODE-ID") == "SKIPPED")
  }

  "Order's start node and end node are the same skipped node" in {
    scheduler executeXml <job_chain_node.modify job_chain="/TEST" state="SKIPPED" action="next_state"/>
    val skipping = notSkipping.copy(orderKey = notSkipping.orderKey.copy(id = OrderId("SKIPPING")))
    interceptSchedulerError(MessageCode("SCHEDULER-438")) {  // "Invalid Job_chain_node for add_order() with nodeId='…'"
      scheduler executeXml skipping
    }
  }

  "Order's end node are the same skipped node" in {
    pending // Still fails
    val skipping = notSkipping.copy(orderKey = notSkipping.orderKey.copy(id = OrderId("SKIPPING-2")), nodeId = None)
    val result = runOrder(skipping)
    assert(result.nodeId == SkippedNodeId && !result.variables.contains("LAST-NODE-ID"))
  }
}

private object JS1844IT
{
  private val jobChainPath = JobChainPath("/TEST")
  private val SkippedNodeId = NodeId("SKIPPED")
}
