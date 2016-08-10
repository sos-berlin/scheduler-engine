package com.sos.scheduler.engine.tests.jira.js1003

import com.sos.scheduler.engine.common.time.JodaJavaTimeConversions.implicits.asJavaInstant
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobChainPath}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand.Action
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.test.{EventPipe, SchedulerTestUtils}
import com.sos.scheduler.engine.test.SchedulerTestUtils.{order, orderOverview}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1003.JS1003IT._
import java.time.Instant
import org.joda.time.DateTime
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1003IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val orderSubsystem = instance[OrderSubsystemClient]
  private lazy val jobChain = orderSubsystem.jobChain(TestJobChainPath)

  "(prepare job chain)" in {
    jobChain.node(State100).action = JobChainNodeAction.nextState
    jobChain.node(State300).action = JobChainNodeAction.stop
  }

  "Resetting scheduled standing order should should wait" in {
    val orderKey = StandingOrderKey
    resetOrderShouldWait(orderKey, scheduledAt = Some(ScheduledStart)) {
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false), at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Reset scheduled non-standing order should wait" in {
    val orderKey = TestJobChainPath orderKey "Scheduled"
    resetOrderShouldWait(orderKey, scheduledAt = Some(ScheduledStart)) {
      val runtime = <run_time><at at="2030-12-31 12:00"/></run_time>
      scheduler executeXml OrderCommand(orderKey, xmlChildren = runtime)
      scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Reset non-standing order should restart should wait" in {
    val orderKey = TestJobChainPath orderKey "Non-scheduled"
    resetOrderShouldWait(orderKey, scheduledAt = None) {
      scheduler executeXml OrderCommand(orderKey)
    }
  }

  "Reset repeating non-standing order should repeat immediately" in {
    val orderKey = TestJobChainPath orderKey "Repeating"
    withEventPipe { implicit eventPipe ⇒
      val runtime = <run_time><period repeat="01:00:00"/></run_time>
      scheduler executeXml OrderCommand(orderKey, xmlChildren = runtime)
      checkBehaviourUntilReset(orderKey)
      eventPipe.nextKeyed[OrderNodeChanged](orderKey).previousState shouldBe State200
    }
  }

  private def resetOrderShouldWait(orderKey: OrderKey, scheduledAt: Option[Instant])(startOrder: ⇒ Unit): Unit = {
    withEventPipe { implicit eventPipe ⇒
      startOrder
      checkBehaviourUntilReset(orderKey)
      order(orderKey).nextInstantOption shouldEqual scheduledAt
      orderOverview(orderKey).orderState shouldBe State200
      Thread.sleep(2000)
      orderOverview(orderKey).orderState shouldBe State200
    }
  }

  private def checkBehaviourUntilReset(orderKey: OrderKey)(implicit eventPipe: EventPipe): Unit = {
    //Wird übersprungen: eventPipe.nextKeyed[OrderNodeChanged](orderKey).previousState shouldBe State100
    eventPipe.nextKeyed[OrderNodeChanged](orderKey).previousState shouldBe State200
    def order = orderSubsystem.orderOverview(orderKey)
    order.orderState shouldBe State300
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(true))
    assert(order.isSuspended)
    scheduler executeXml ModifyOrderCommand(orderKey, action = Some(Action.reset))
    assert(!order.isSuspended)
    eventPipe.nextKeyed[OrderNodeChanged](orderKey).previousState shouldBe State300
  }
}

private object JS1003IT {
  private val ScheduledStart = asJavaInstant(new DateTime(2030, 12, 31, 12, 0).toInstant)
  private val TestJobChainPath = JobChainPath("/test")
  private val State100 = OrderState("100")
  private val State200 = OrderState("200")
  private val State300 = OrderState("300")
  private val StandingOrderKey = TestJobChainPath orderKey "1"
}
