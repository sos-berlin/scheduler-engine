package com.sos.scheduler.engine.tests.jira.js631

import JS631IT._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderStateChangedEvent, OrderState}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS631IT extends FunSuite with ScalaSchedulerTest {
  override protected lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    database = Some(DefaultDatabaseConfiguration()))

  test("Without reset") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <modify_order job_chain={aOrderKey.jobChainPath.string} order={aOrderKey.id.string} at="now"/>
    eventPipe.nextKeyed[OrderStateChangedEvent](aOrderKey).previousState should equal (a1State)
    eventPipe.nextKeyed[OrderStateChangedEvent](aOrderKey).previousState should equal (a2State)
    eventPipe.nextKeyed[OrderStateChangedEvent](bOrderKey).previousState should equal (b1State)
    eventPipe.nextKeyed[OrderStateChangedEvent](bOrderKey).previousState should equal (b2State)
    eventPipe.nextKeyed[OrderStateChangedEvent](bOrderKey).previousState should equal (b3State)
    eventPipe.nextKeyed[OrderFinishedEvent](bOrderKey)
  }

  test("Reset in second nested job chain") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <job_chain_node.modify job_chain={bJobChainPath.string} state={b2State.string} action="stop"/>
    scheduler executeXml <modify_order job_chain={aOrderKey.jobChainPath.string} order={aOrderKey.id.string} at="now"/>
    eventPipe.nextKeyed[OrderStateChangedEvent](aOrderKey).previousState should equal (a1State)
    eventPipe.nextKeyed[OrderStateChangedEvent](aOrderKey).previousState should equal (a2State)
    eventPipe.nextKeyed[OrderStateChangedEvent](bOrderKey).previousState should equal (b1State)
    instance[OrderSubsystem].order(bOrderKey).nextInstantOption should equal (None)
    scheduler executeXml <modify_order job_chain={bOrderKey.jobChainPath.string} order={bOrderKey.id.string} action="reset"/>  // Fehler: SCHEDULER-149  There is no job in job chain "/test-nested-b" for the state "A"
    instance[OrderSubsystem].order(aOrderKey).nextInstantOption should equal (None)
    instance[OrderSubsystem].order(aOrderKey).state should be (a1State)
  }

  test("Reset in first nested job chain (initial state is in current job chain)") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <job_chain_node.modify job_chain={aJobChainPath.string} state={a2State.string} action="stop"/>
    scheduler executeXml <modify_order job_chain={aOrderKey.jobChainPath.string} order={aOrderKey.id.string} at="now"/>
    eventPipe.nextKeyed[OrderStateChangedEvent](aOrderKey).previousState should equal (a1State)
    instance[OrderSubsystem].order(aOrderKey).nextInstantOption should equal (None)
    scheduler executeXml <modify_order job_chain={aOrderKey.jobChainPath.string} order={aOrderKey.id.string} action="reset"/>
    instance[OrderSubsystem].order(aOrderKey).nextInstantOption should equal (None)
  }
}

object JS631IT {
  private val aJobChainPath = JobChainPath("/test-nested-a")
  private val bJobChainPath = JobChainPath("/test-nested-b")
  private val aOrderKey = aJobChainPath orderKey "1"
  private val bOrderKey = bJobChainPath orderKey "1"
  private val a1State = OrderState("A-1")
  private val a2State = OrderState("A-2")
  private val b1State = OrderState("B-1")
  private val b2State = OrderState("B-2")
  private val b3State = OrderState("B-3")
}
