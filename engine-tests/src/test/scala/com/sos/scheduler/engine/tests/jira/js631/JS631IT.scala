package com.sos.scheduler.engine.tests.jira.js631

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderNestedFinished, OrderNestedStarted, OrderState, OrderNodeChanged, OrderStarted}
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
      eventPipe.nextKeyed[OrderStarted](AOrderKey)
      eventPipe.nextKeyed[OrderNestedStarted](AOrderKey)
      eventPipe.nextKeyed[OrderNodeChanged](AOrderKey).previousState shouldEqual A1State
      eventPipe.nextKeyed[OrderNodeChanged](AOrderKey).previousState shouldEqual A2State
      eventPipe.nextKeyed[OrderNestedFinished](AOrderKey)
      eventPipe.nextKeyed[OrderNestedStarted](BOrderKey)
      eventPipe.nextKeyed[OrderNodeChanged](BOrderKey).previousState shouldEqual B1State
      eventPipe.nextKeyed[OrderNodeChanged](BOrderKey).previousState shouldEqual B2State
      eventPipe.nextKeyed[OrderNodeChanged](BOrderKey).previousState shouldEqual B3State
      eventPipe.nextKeyed[OrderNestedFinished](BOrderKey)
      eventPipe.nextKeyed[OrderFinished](BOrderKey)
    }
  }

  "Reset in second nested job chain with first nested job chain completely skipping (JS-1476)" in {
    withEventPipe { eventPipe ⇒
      scheduler executeXml <job_chain_node.modify job_chain={BJobChainPath.string} state={B2State.string} action="stop"/>
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.nextKeyed[OrderNodeChanged](AOrderKey).previousState shouldEqual A1State
      eventPipe.nextKeyed[OrderNodeChanged](AOrderKey).previousState shouldEqual A2State
      eventPipe.nextKeyed[OrderNodeChanged](BOrderKey).previousState shouldEqual B1State
      order(BOrderKey).nextInstantOption shouldEqual None
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A1State.string} action="next_state"/>
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A2State.string} action="next_state"/>
      scheduler executeXml <modify_order job_chain={BOrderKey.jobChainPath.string} order={BOrderKey.id.string} action="reset"/>
      order(BOrderKey).nextInstantOption shouldEqual None
      orderOverview(BOrderKey).orderState should be(B1State)
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A1State.string} action="process"/>
      scheduler executeXml <job_chain_node.modify job_chain={AOrderKey.jobChainPath.string} state={A2State.string} action="process"/>
    }
  }

  "Reset in second nested job chain" in {
    withEventPipe { eventPipe ⇒
      order(BOrderKey).nextInstantOption shouldEqual None
      orderOverview(BOrderKey).orderState should be(B1State)
      scheduler executeXml <modify_order job_chain={BOrderKey.jobChainPath.string} order={BOrderKey.id.string} action="reset"/> // Was error: SCHEDULER-149  There is no job in job chain "/test-nested-b" for the state "A"
      order(AOrderKey).nextInstantOption shouldEqual None
      orderOverview(AOrderKey).orderState should be(A1State)
    }
  }

  "Reset in first nested job chain (initial state is in current job chain)" in {
    withEventPipe { eventPipe ⇒
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={A2State.string} action="stop"/>
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.nextKeyed[OrderNodeChanged](AOrderKey).previousState shouldEqual A1State
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
  private val A1State = OrderState("A-1")
  private val A2State = OrderState("A-2")
  private val B1State = OrderState("B-1")
  private val B2State = OrderState("B-2")
  private val B3State = OrderState("B-3")
}
