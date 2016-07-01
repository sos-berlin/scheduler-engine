package com.sos.scheduler.engine.tests.order.events

import com.sos.scheduler.engine.common.scalautil.Collections.implicits.InsertableMutableMap
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.eventbus.{EventHandler, HotEventHandler}
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.order.events.OrderEventsIT._
import java.lang.Thread.sleep
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/** Testet, ob die erwarteten Auftrags-Events in der richtigen Reihenfolge eintreffen,
 * außerdem @EventHandler und @HotEventHandler. */
@RunWith(classOf[JUnitRunner])
final class OrderEventsIT extends FreeSpec with ScalaSchedulerTest {

  private val eventPipe = controller.newEventPipe()
  private val hotEvents = mutable.Map[String, Event]()
  private val coldEvents = mutable.Map[String, Event]()

  "Persistent order" in {
    checkOrderStates(PersistentOrderKey)
    eventPipe.nextAny[OrderEvent] match {
      case e: OrderStateChangedEvent ⇒
        e.orderKey shouldEqual PersistentOrderKey
        assert(e.previousState == OrderState("end"))
    }
  }

  "Temporary order" in {
    hotEvents.clear()
    coldEvents.clear()
    scheduler.executeXml(<add_order job_chain={TestJobChainPath.string} id={TemporaryOrderKey.id.toString}/>)
    checkOrderStates(TemporaryOrderKey)
  }

  "Removed temporary order" in {
    hotEvents.clear()
    coldEvents.clear()
    val orderKey = TemporaryOrderKey
    scheduler.executeXml(<add_order job_chain={TestJobChainPath.string} id={orderKey.id.toString}/>)
    eventPipe.nextAny[OrderTouchedEvent].orderKey shouldEqual orderKey
    eventPipe.nextAny[OrderSuspendedEvent].orderKey shouldEqual orderKey
    eventPipe.nextAny[OrderEvent] match { case e: OrderStateChangedEvent ⇒ e.orderKey shouldEqual orderKey; e.previousState shouldEqual OrderState("state1") }
    scheduler.executeXml(<remove_order job_chain={TestJobChainPath.string} order={orderKey.id.toString}/>)
    //eventPipe.next[OrderEvent] match { case e: OrderFinishedEvent ⇒ e.getKey shouldEqual orderKey }
  }

  private def checkOrderStates(orderKey: OrderKey): Unit = {
    expectOrderEventsAndResumeOrder(orderKey)
    checkCollectedOrderEvents(orderKey)
  }

  private def expectOrderEventsAndResumeOrder(orderKey: OrderKey): Unit = {
    eventPipe.nextAny[OrderEvent] match { case e: OrderTouchedEvent ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepStartedEvent ⇒ e.orderKey shouldEqual orderKey; e.state shouldEqual OrderState("state1") }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepEndedEvent ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderSuspendedEvent ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStateChangedEvent ⇒ e.orderKey shouldEqual orderKey; e.previousState shouldEqual OrderState("state1") }
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
    eventPipe.nextAny[OrderEvent] match { case e: OrderResumedEvent ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepStartedEvent ⇒ e.orderKey shouldEqual orderKey; e.state shouldEqual OrderState("state2") }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepEndedEvent ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStateChangedEvent ⇒ e.orderKey shouldEqual orderKey; e.previousState shouldEqual OrderState("state2") }
    eventPipe.nextAny[OrderEvent] match { case e: OrderFinishedEvent ⇒ e.orderKey shouldEqual orderKey }
  }

  private def checkCollectedOrderEvents(orderKey: OrderKey): Unit = {
    sleep(500)  // The last event can be late???
    eventBus.dispatchEvents()  // Das letzte OrderFinishedEvent kann sonst verloren gehen.
    sleep(500)  // The last event can be late???
    coldEvents.toMap shouldEqual Map(
      "OrderTouched"   → OrderTouchedEvent(orderKey),
      "OrderFinished"  → OrderFinishedEvent(orderKey, OrderState("end")),
      "OrderSuspended" → OrderSuspendedEvent(orderKey),
      "OrderResumed"   → OrderResumedEvent(orderKey)
    )
    hotEvents shouldEqual Map(
      "OrderTouched UnmodifiableOrder"            → OrderTouchedEvent(orderKey),
      "OrderTouched Order"                        → OrderTouchedEvent(orderKey),
      "OrderFinished UnmodifiableOrder"           → OrderFinishedEvent(orderKey, OrderState("end")),
      "OrderStepStarted UnmodifiableOrder state1" → OrderStepStartedEvent(orderKey, OrderState("state1"), TaskId.Null),
      "OrderStepStarted Order state1"             → OrderStepStartedEvent(orderKey, OrderState("state1"), TaskId.Null),
      "OrderStepEnded UnmodifiableOrder state1"   → OrderStepEndedEvent(orderKey, SuccessOrderStateTransition),
      "OrderStepEnded Order state1"               → OrderStepEndedEvent(orderKey, SuccessOrderStateTransition),
      "OrderStepStarted UnmodifiableOrder state2" → OrderStepStartedEvent(orderKey, OrderState("state2"), TaskId.Null),
      "OrderStepStarted Order state2"             → OrderStepStartedEvent(orderKey, OrderState("state2"), TaskId.Null),
      "OrderStepEnded UnmodifiableOrder state2"   → OrderStepEndedEvent(orderKey, SuccessOrderStateTransition),
      "OrderStepEnded Order state2"               → OrderStepEndedEvent(orderKey, SuccessOrderStateTransition),
      "OrderSuspended UnmodifiableOrder"          → OrderSuspendedEvent(orderKey),
      "OrderResumed UnmodifiableOrder"            → OrderResumedEvent(orderKey))
  }

  @EventHandler def handleEvent(e: OrderTouchedEvent): Unit =
    addEvent(coldEvents, "OrderTouched" → e)

  @HotEventHandler def handleHotEvent(e: OrderTouchedEvent, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderTouched UnmodifiableOrder" → e)

  @HotEventHandler def handleHotEvent(e: OrderTouchedEvent, o: Order): Unit =
    addEvent(hotEvents, "OrderTouched Order" → e)

  @EventHandler def handleEvent(e: OrderFinishedEvent): Unit =
    addEvent(coldEvents, "OrderFinished" → e)

  @HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderFinished UnmodifiableOrder" → e)

  @HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: Order): Unit =
    fail("@HotEventHandler def handleHotEvent(e: OrderFinishedEvent, o: Order) should not be called because of UnmodifiableOrder")

  @HotEventHandler def handleHotEvent(e: OrderStepStartedEvent, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, s"OrderStepStarted UnmodifiableOrder ${o.state}" → e.copy(taskId = TaskId.Null))

  @HotEventHandler def handleHotEvent(e: OrderStepStartedEvent, o: Order): Unit =
    addEvent(hotEvents, s"OrderStepStarted Order ${o.state}" → e.copy(taskId = TaskId.Null))

  @HotEventHandler def handleHotEvent(e: OrderStepEndedEvent, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, s"OrderStepEnded UnmodifiableOrder ${o.state}" → e)

  @HotEventHandler def handleHotEvent(e: OrderStepEndedEvent, o: Order): Unit =
    addEvent(hotEvents, s"OrderStepEnded Order ${o.state}" → e)

  @EventHandler def handleEvent(e: OrderSuspendedEvent): Unit =
    addEvent(coldEvents, "OrderSuspended" → e)

  @HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderSuspended UnmodifiableOrder" → e)

  @HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: Order): Unit =
    fail("@HotEventHandler def handleHotEvent(e: OrderSuspendedEvent, o: Order) should not be called because of UnmodifiableOrder")

  @EventHandler def handleEvent(e: OrderResumedEvent): Unit =
    addEvent(coldEvents, "OrderResumed" → e)

  @HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderResumed UnmodifiableOrder" → e)

  @HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: Order): Unit =
    fail("@HotEventHandler def handleHotEvent(e: OrderResumedEvent, o: Order) should not be called because of UnmodifiableOrder")

  private def addEvent(m: mutable.Map[String,Event], pair: (String, Event)): Unit =
    m.synchronized {
      m.insert(pair)
    }
}


private object OrderEventsIT {
  private val TestJobChainPath = JobChainPath("/a")
  private val PersistentOrderKey = TestJobChainPath orderKey "persistentOrder"
  private val TemporaryOrderKey = TestJobChainPath orderKey "1"
}
