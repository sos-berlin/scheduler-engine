package com.sos.scheduler.engine.tests.jira.js631

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderNestedFinishedEvent, OrderNestedTouchedEvent, OrderState, OrderStateChangedEvent, OrderTouchedEvent}
import com.sos.scheduler.engine.test.SchedulerTestUtils.order
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.jira.js631.JS631IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS631IT extends FreeSpec with ScalaSchedulerTest {

  "Without reset" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.nextKeyed[OrderTouchedEvent](AOrderKey)
      eventPipe.nextKeyed[OrderNestedTouchedEvent](AOrderKey)
      eventPipe.nextKeyed[OrderStateChangedEvent](AOrderKey).previousState shouldEqual A1State
      eventPipe.nextKeyed[OrderStateChangedEvent](AOrderKey).previousState shouldEqual A2State
      eventPipe.nextKeyed[OrderNestedFinishedEvent](AOrderKey)
      eventPipe.nextKeyed[OrderNestedTouchedEvent](BOrderKey)
      eventPipe.nextKeyed[OrderStateChangedEvent](BOrderKey).previousState shouldEqual B1State
      eventPipe.nextKeyed[OrderStateChangedEvent](BOrderKey).previousState shouldEqual B2State
      eventPipe.nextKeyed[OrderStateChangedEvent](BOrderKey).previousState shouldEqual B3State
      eventPipe.nextKeyed[OrderNestedFinishedEvent](BOrderKey)
      eventPipe.nextKeyed[OrderFinishedEvent](BOrderKey)
    }
  }

  "Reset in second nested job chain" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      scheduler executeXml <job_chain_node.modify job_chain={BJobChainPath.string} state={B2State.string} action="stop"/>
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.nextKeyed[OrderStateChangedEvent](AOrderKey).previousState shouldEqual A1State
      eventPipe.nextKeyed[OrderStateChangedEvent](AOrderKey).previousState shouldEqual A2State
      eventPipe.nextKeyed[OrderStateChangedEvent](BOrderKey).previousState shouldEqual B1State
      order(BOrderKey).nextInstantOption shouldEqual None
      scheduler executeXml <modify_order job_chain={BOrderKey.jobChainPath.string} order={BOrderKey.id.string} action="reset"/> // Fehler: SCHEDULER-149  There is no job in job chain "/test-nested-b" for the state "A"
      order(AOrderKey).nextInstantOption shouldEqual None
      order(AOrderKey).state should be(A1State)
    }
  }

  "Reset in first nested job chain (initial state is in current job chain)" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      scheduler executeXml <job_chain_node.modify job_chain={AJobChainPath.string} state={A2State.string} action="stop"/>
      scheduler executeXml <modify_order job_chain={AOrderKey.jobChainPath.string} order={AOrderKey.id.string} at="now"/>
      eventPipe.nextKeyed[OrderStateChangedEvent](AOrderKey).previousState shouldEqual A1State
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
