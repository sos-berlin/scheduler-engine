package com.sos.scheduler.engine.tests.jira.js631

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderNestedFinished, OrderNestedStarted, OrderNodeChanged, OrderStarted}
import com.sos.scheduler.engine.test.SchedulerTestUtils.{order, orderOverview}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js631.JS631IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-631 &lt;<modify_order action="reset"/> doesn't work in nested job chains.
 * <p>
 * JS-1476 Completely skipping nested jobchains.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS631IT extends FreeSpec with ScalaSchedulerTest {

  "Without reset" in {
    withEventPipe { eventPipe ⇒
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.next[OrderStarted.type](AOrderKey)
      eventPipe.next[OrderNestedStarted.type](AOrderKey)
      eventPipe.next[OrderNodeChanged](AOrderKey).fromNodeId shouldEqual A1NodeId
      eventPipe.next[OrderNodeChanged](AOrderKey).fromNodeId shouldEqual A2NodeId
      eventPipe.next[OrderNestedFinished.type](AOrderKey)
      eventPipe.next[OrderNestedStarted.type](BOrderKey)
      eventPipe.next[OrderNodeChanged](BOrderKey).fromNodeId shouldEqual B1NodeId
      eventPipe.next[OrderNodeChanged](BOrderKey).fromNodeId shouldEqual B2NodeId
      eventPipe.next[OrderNodeChanged](BOrderKey).fromNodeId shouldEqual B3NodeId
      eventPipe.next[OrderNestedFinished.type](BOrderKey)
      eventPipe.next[OrderFinished](BOrderKey)
    }
  }

  "Reset in second nested job chain with first nested job chain completely skipping (JS-1476)" in {
    withEventPipe { eventPipe ⇒
      scheduler executeXml <job_chain_node.modify job_chain={BJobChainPath.string} state={B2NodeId.string} action="stop"/>
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.next[OrderNodeChanged](AOrderKey).fromNodeId shouldEqual A1NodeId
      eventPipe.next[OrderNodeChanged](AOrderKey).fromNodeId shouldEqual A2NodeId
      eventPipe.next[OrderNodeChanged](BOrderKey).fromNodeId shouldEqual B1NodeId
      order(BOrderKey).nextInstantOption shouldEqual None
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A1NodeId.string} action="next_state"/>
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A2NodeId.string} action="next_state"/>
      scheduler executeXml <modify_order job_chain={BOrderKey.jobChainPath.string} order={BOrderKey.id.string} action="reset"/>
      order(BOrderKey).nextInstantOption shouldEqual None
      orderOverview(BOrderKey).nodeId should be(B1NodeId)
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A1NodeId.string} action="process"/>
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A2NodeId.string} action="process"/>
    }
  }

  "Reset in second nested job chain" in {
    withEventPipe { eventPipe ⇒
      order(BOrderKey).nextInstantOption shouldEqual None
      orderOverview(BOrderKey).nodeId should be(B1NodeId)
      scheduler executeXml <modify_order job_chain={BOrderKey.jobChainPath.string} order={BOrderKey.id.string} action="reset"/> // Was error: SCHEDULER-149  There is no job in job chain "/test-nested-b" for the nodeId "A"
      order(AOrderKey).nextInstantOption shouldEqual None
      orderOverview(AOrderKey).nodeId should be(A1NodeId)
    }
  }

  "Reset in first nested job chain (initial nodeId is in current job chain)" in {
    withEventPipe { eventPipe ⇒
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={A2NodeId.string} action="stop"/>
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.next[OrderNodeChanged](AOrderKey).fromNodeId shouldEqual A1NodeId
      order(AOrderKey).nextInstantOption shouldEqual None
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} action="reset"/>
      order(AOrderKey).nextInstantOption shouldEqual None
    }
  }
}

private object JS631IT {
  private val AJobChainPath = JobChainPath("/test-nested-a")
  private val BJobChainPath = JobChainPath("/test-nested-b")
  private val AOrderKey = AJobChainPath orderKey "1"
  private val BOrderKey = BJobChainPath orderKey "1"
  private val A1NodeId = NodeId("A-1")
  private val A2NodeId = NodeId("A-2")
  private val B1NodeId = NodeId("B-1")
  private val B2NodeId = NodeId("B-2")
  private val B3NodeId = NodeId("B-3")
}
