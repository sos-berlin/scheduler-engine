package com.sos.scheduler.engine.tests.order.events

import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.folder.AbsolutePath
import com.sos.scheduler.engine.eventbus.{HotEventHandler, EventHandler}
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import com.sos.scheduler.engine.data.order._

/** Testet, ob die erwarteten Auftrags-Events in der richtigen Reihenfolge eintreffen,
 * außerdem @EventHandler und @HotEventHandler. */
@RunWith(classOf[JUnitRunner])
class OrderEventsTest extends ScalaSchedulerTest {
  import OrderEventsTest._

  private val eventPipe = controller.newEventPipe()
  private val hotEvents = new mutable.HashMap[String,Event]
  private val coldEvents = new mutable.HashMap[String,Event]

  test("Persistent order") {
    checkOrderStates(persistentOrderKey)
  }

  test("Temporary order") {
    hotEvents.clear()
    coldEvents.clear()
    scheduler.executeXml(<add_order job_chain={jobChainPath.toString} id={temporaryOrderKey.getId.toString}/>)
    checkOrderStates(temporaryOrderKey)
  }

  test("Removed temporary order") {
    hotEvents.clear()
    coldEvents.clear()
    val orderKey = temporaryOrderKey
    scheduler.executeXml(<add_order job_chain={jobChainPath.toString} id={orderKey.getId.toString}/>)
    eventPipe.expectEvent(shortTimeout) { e: OrderTouchedEvent => e.getKey == orderKey }
    eventPipe.expectEvent(shortTimeout) { e: OrderSuspendedEvent => e.getKey == orderKey }
    scheduler.executeXml(<remove_order job_chain={jobChainPath.toString} order={orderKey.getId.toString}/>)
    //eventPipe.expectEvent(shortTimeout) { e: OrderFinishedEvent => e.getKey == orderKey }
  }

  private def checkOrderStates(orderKey: OrderKey) {
    expectOrderEventsAndResumeOrder(orderKey)
    checkCollectedOrderEvents(orderKey)
  }

  private def expectOrderEventsAndResumeOrder(orderKey: OrderKey) {
    eventPipe.expectEvent(shortTimeout) { e: OrderTouchedEvent => e.getKey == orderKey }
    eventPipe.expectEvent(shortTimeout) { e: OrderSuspendedEvent => e.getKey == orderKey }
    scheduler.executeXml(<modify_order job_chain={jobChainPath.toString} order={orderKey.getId.toString} suspended="no"/>)
    eventPipe.expectEvent(shortTimeout) { e: OrderResumedEvent => e.getKey == orderKey }
    eventPipe.expectEvent(shortTimeout) { e: OrderFinishedEvent => e.getKey == orderKey }
  }

  private def checkCollectedOrderEvents(orderKey: OrderKey) {
    controller.getEventBus.dispatchEvents()  // Das letzte OrderFinishedEvent kann sonst verloren gehen.
    coldEvents should equal (Map(
      "OrderTouched"   -> new OrderTouchedEvent(orderKey),
      "OrderFinished"  -> new OrderFinishedEvent(orderKey),
      "OrderSuspended" -> new OrderSuspendedEvent(orderKey),
      "OrderResumed"   -> new OrderResumedEvent(orderKey)
    ))
    hotEvents should equal (Map(
      "OrderTouched UnmodifiableOrder"            -> new OrderTouchedEvent(orderKey),
      "OrderTouched Order"                        -> new OrderTouchedEvent(orderKey),
      "OrderFinished UnmodifiableOrder"           -> new OrderFinishedEvent(orderKey),
      "OrderStepStarted UnmodifiableOrder state1" -> new OrderStepStartedEvent(orderKey),
      "OrderStepStarted Order state1"             -> new OrderStepStartedEvent(orderKey),
      "OrderStepEnded UnmodifiableOrder state1"   -> new OrderStepEndedEvent(orderKey, true),
      "OrderStepEnded Order state1"               -> new OrderStepEndedEvent(orderKey, true),
      "OrderStepStarted UnmodifiableOrder state2" -> new OrderStepStartedEvent(orderKey),
      "OrderStepStarted Order state2"             -> new OrderStepStartedEvent(orderKey),
      "OrderStepEnded UnmodifiableOrder state2"   -> new OrderStepEndedEvent(orderKey, true),
      "OrderStepEnded Order state2"               -> new OrderStepEndedEvent(orderKey, true),
      "OrderSuspended UnmodifiableOrder"          -> new OrderSuspendedEvent(orderKey),
      "OrderResumed UnmodifiableOrder"            -> new OrderResumedEvent(orderKey)))
  }

  @EventHandler def handleEvent(e: OrderTouchedEvent) {
    addEvent(coldEvents, "OrderTouched" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderTouchedEvent, o: UnmodifiableOrder) {
    addEvent(hotEvents, "OrderTouched UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderTouchedEvent, o: Order) {
    addEvent(hotEvents, "OrderTouched Order" -> e)
  }

  @EventHandler def handleEvent(e: OrderFinishedEvent) {
    addEvent(coldEvents, "OrderFinished" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: UnmodifiableOrder) {
    addEvent(hotEvents, "OrderFinished UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: Order) {
    fail("@HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: Order) should not be called because of UnmodifiableOrder")
  }

  @HotEventHandler def handleHotEvent(e: OrderStepStartedEvent, o: UnmodifiableOrder) {
    addEvent(hotEvents, "OrderStepStarted UnmodifiableOrder "+ o.getState -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderStepStartedEvent, o: Order) {
    addEvent(hotEvents, "OrderStepStarted Order "+ o.getState -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderStepEndedEvent, o: UnmodifiableOrder) {
    addEvent(hotEvents, "OrderStepEnded UnmodifiableOrder "+ o.getState -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderStepEndedEvent, o: Order) {
    addEvent(hotEvents, "OrderStepEnded Order "+ o.getState -> e)
  }

  @EventHandler def handleEvent(e: OrderSuspendedEvent) {
    addEvent(coldEvents, "OrderSuspended" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: UnmodifiableOrder) {
    addEvent(hotEvents, "OrderSuspended UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: Order) {
    fail("@HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: Order) should not be called because of UnmodifiableOrder")
  }

  @EventHandler def handleEvent(e: OrderResumedEvent) {
    addEvent(coldEvents, "OrderResumed" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: UnmodifiableOrder) {
    addEvent(hotEvents, "OrderResumed UnmodifiableOrder" -> e)
  }

  @HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: Order) {
    fail("@HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: Order) should not be called because of UnmodifiableOrder")
  }

  private def addEvent(m: mutable.HashMap[String,Event], pair: (String, Event)) {
    val (name, e) = pair
    assert(!m.contains(name), "Duplicate event "+ name +": "+ e)
    m(name) = e
  }
}

object OrderEventsTest {
  private val jobChainPath = new AbsolutePath("/a")
  private val persistentOrderKey = new OrderKey(jobChainPath, new OrderId("persistentOrder"))
  private val temporaryOrderKey = new OrderKey(jobChainPath, new OrderId("1"))
}
