package com.sos.scheduler.engine.tests.order.events

import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.{EventHandler, HotEventHandler}
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.order.events.OrderEventsIT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/** Testet, ob die erwarteten Auftrags-Events in der richtigen Reihenfolge eintreffen,
 * außerdem @EventHandler und @HotEventHandler. */
@RunWith(classOf[JUnitRunner])
final class OrderEventsIT extends FunSuite with ScalaSchedulerTest {

  private val eventPipe = controller.newEventPipe()
  private val hotEvents = new mutable.HashMap[String, Event]
  private val coldEvents = new mutable.HashMap[String, Event]

  test("Persistent order") {
    checkOrderStates(persistentOrderKey)
    eventPipe.nextAny[OrderEvent] match { case e: OrderStateChangedEvent => e.orderKey should equal (persistentOrderKey); assert(e.previousState === OrderState("end")) }
  }

  test("Temporary order") {
    hotEvents.clear()
    coldEvents.clear()
    scheduler.executeXml(<add_order job_chain={jobChainPath.string} id={temporaryOrderKey.id.toString}/>)
    checkOrderStates(temporaryOrderKey)
  }

  test("Removed temporary order") {
    hotEvents.clear()
    coldEvents.clear()
    val orderKey = temporaryOrderKey
    scheduler.executeXml(<add_order job_chain={jobChainPath.string} id={orderKey.id.toString}/>)
    eventPipe.nextAny[OrderTouchedEvent].orderKey should equal (orderKey)
    eventPipe.nextAny[OrderSuspendedEvent].orderKey should equal (orderKey)
    eventPipe.nextAny[OrderEvent] match { case e: OrderStateChangedEvent => e.orderKey should equal (orderKey); e.previousState should equal (OrderState("state1")) }
    scheduler.executeXml(<remove_order job_chain={jobChainPath.string} order={orderKey.id.toString}/>)
    //eventPipe.next[OrderEvent] match { case e: OrderFinishedEvent => e.getKey should equal (orderKey) }
  }

  private def checkOrderStates(orderKey: OrderKey): Unit = {
    expectOrderEventsAndResumeOrder(orderKey)
    checkCollectedOrderEvents(orderKey)
  }

  private def expectOrderEventsAndResumeOrder(orderKey: OrderKey): Unit = {
    eventPipe.nextAny[OrderEvent] match { case e: OrderTouchedEvent => e.orderKey should equal (orderKey) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepStartedEvent => e.orderKey should equal (orderKey); e.state should equal (OrderState("state1")) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepEndedEvent => e.orderKey should equal (orderKey) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderSuspendedEvent => e.orderKey should equal (orderKey) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStateChangedEvent => e.orderKey should equal (orderKey); e.previousState should equal (OrderState("state1")) }
    scheduler.executeXml(<modify_order job_chain={jobChainPath.string} order={orderKey.id.toString} suspended="no"/>)
    eventPipe.nextAny[OrderEvent] match { case e: OrderResumedEvent => e.orderKey should equal (orderKey) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepStartedEvent => e.orderKey should equal (orderKey); e.state should equal (OrderState("state2")) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepEndedEvent => e.orderKey should equal (orderKey) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStateChangedEvent => e.orderKey should equal (orderKey); e.previousState should equal (OrderState("state2")) }
    eventPipe.nextAny[OrderEvent] match { case e: OrderFinishedEvent => e.orderKey should equal (orderKey) }
  }

  private def checkCollectedOrderEvents(orderKey: OrderKey): Unit = {
    controller.getEventBus.dispatchEvents()  // Das letzte OrderFinishedEvent kann sonst verloren gehen.
    coldEvents should equal (Map(
      "OrderTouched"   -> OrderTouchedEvent(orderKey),
      "OrderFinished"  -> OrderFinishedEvent(orderKey),
      "OrderSuspended" -> OrderSuspendedEvent(orderKey),
      "OrderResumed"   -> OrderResumedEvent(orderKey)
    ))
    hotEvents should equal (Map(
      "OrderTouched UnmodifiableOrder"            -> OrderTouchedEvent(orderKey),
      "OrderTouched Order"                        -> OrderTouchedEvent(orderKey),
      "OrderFinished UnmodifiableOrder"           -> OrderFinishedEvent(orderKey),
      "OrderStepStarted UnmodifiableOrder state1" -> OrderStepStartedEvent(orderKey, OrderState("state1")),
      "OrderStepStarted Order state1"             -> OrderStepStartedEvent(orderKey, OrderState("state1")),
      "OrderStepEnded UnmodifiableOrder state1"   -> OrderStepEndedEvent(orderKey, OrderStateTransition.success),
      "OrderStepEnded Order state1"               -> OrderStepEndedEvent(orderKey, OrderStateTransition.success),
      "OrderStepStarted UnmodifiableOrder state2" -> OrderStepStartedEvent(orderKey, OrderState("state2")),
      "OrderStepStarted Order state2"             -> OrderStepStartedEvent(orderKey, OrderState("state2")),
      "OrderStepEnded UnmodifiableOrder state2"   -> OrderStepEndedEvent(orderKey, OrderStateTransition.success),
      "OrderStepEnded Order state2"               -> OrderStepEndedEvent(orderKey, OrderStateTransition.success),
      "OrderSuspended UnmodifiableOrder"          -> OrderSuspendedEvent(orderKey),
      "OrderResumed UnmodifiableOrder"            -> OrderResumedEvent(orderKey)))
  }

  @EventHandler def handleEvent(e: OrderTouchedEvent): Unit = {
    addEvent(coldEvents, "OrderTouched" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderTouchedEvent, o: UnmodifiableOrder): Unit = {
    addEvent(hotEvents, "OrderTouched UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderTouchedEvent, o: Order): Unit = {
    addEvent(hotEvents, "OrderTouched Order" -> e)
  }

  @EventHandler def handleEvent(e: OrderFinishedEvent): Unit = {
    addEvent(coldEvents, "OrderFinished" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: UnmodifiableOrder): Unit = {
    addEvent(hotEvents, "OrderFinished UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: Order): Unit = {
    fail("@HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: Order) should not be called because of UnmodifiableOrder")
  }

  @HotEventHandler def handleHotEvent(e: OrderStepStartedEvent, o: UnmodifiableOrder): Unit = {
    addEvent(hotEvents, "OrderStepStarted UnmodifiableOrder "+ o.state -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderStepStartedEvent, o: Order): Unit = {
    addEvent(hotEvents, "OrderStepStarted Order "+ o.state -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderStepEndedEvent, o: UnmodifiableOrder): Unit = {
    addEvent(hotEvents, "OrderStepEnded UnmodifiableOrder "+ o.state -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderStepEndedEvent, o: Order): Unit = {
    addEvent(hotEvents, "OrderStepEnded Order "+ o.state -> e)
  }

  @EventHandler def handleEvent(e: OrderSuspendedEvent): Unit = {
    addEvent(coldEvents, "OrderSuspended" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: UnmodifiableOrder): Unit = {
    addEvent(hotEvents, "OrderSuspended UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: Order): Unit = {
    fail("@HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: Order) should not be called because of UnmodifiableOrder")
  }

  @EventHandler def handleEvent(e: OrderResumedEvent): Unit = {
    addEvent(coldEvents, "OrderResumed" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: UnmodifiableOrder): Unit = {
    addEvent(hotEvents, "OrderResumed UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: Order): Unit = {
    fail("@HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: Order) should not be called because of UnmodifiableOrder")
  }

  private def addEvent(m: mutable.HashMap[String,Event], pair: (String, Event)): Unit = {
    m.synchronized {
      val (name, e) = pair
      assert(!m.contains(name), "Duplicate event " + name + ": " + e)
      m(name) = e
    }
  }
}


private object OrderEventsIT {
  private val jobChainPath = JobChainPath("/a")
  private val persistentOrderKey = jobChainPath orderKey "persistentOrder"
  private val temporaryOrderKey = jobChainPath orderKey "1"
}
