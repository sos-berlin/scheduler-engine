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

  "Permanent order" in {
    checkOrderStates(PersistentOrderKey)
    eventPipe.nextAny[OrderEvent] match {
      case e: OrderNodeChanged ⇒
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
    eventPipe.nextAny[OrderStarted].orderKey shouldEqual orderKey
    eventPipe.nextAny[OrderSuspended].orderKey shouldEqual orderKey
    eventPipe.nextAny[OrderEvent] match { case e: OrderNodeChanged ⇒ e.orderKey shouldEqual orderKey; e.previousState shouldEqual OrderState("state1") }
    scheduler.executeXml(<remove_order job_chain={TestJobChainPath.string} order={orderKey.id.toString}/>)
    //eventPipe.next[OrderEvent] match { case e: OrderFinished ⇒ e.getKey shouldEqual orderKey }
  }

  private def checkOrderStates(orderKey: OrderKey): Unit = {
    expectOrderEventsAndResumeOrder(orderKey)
    checkCollectedOrderEvents(orderKey)
  }

  private def expectOrderEventsAndResumeOrder(orderKey: OrderKey): Unit = {
    eventPipe.nextAny[OrderEvent] match { case e: OrderStarted ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepStarted ⇒ e.orderKey shouldEqual orderKey; e.state shouldEqual OrderState("state1") }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepEnded ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderSuspended ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderNodeChanged ⇒ e.orderKey shouldEqual orderKey; e.previousState shouldEqual OrderState("state1") }
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
    eventPipe.nextAny[OrderEvent] match { case e: OrderResumed ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepStarted ⇒ e.orderKey shouldEqual orderKey; e.state shouldEqual OrderState("state2") }
    eventPipe.nextAny[OrderEvent] match { case e: OrderStepEnded ⇒ e.orderKey shouldEqual orderKey }
    eventPipe.nextAny[OrderEvent] match { case e: OrderNodeChanged ⇒ e.orderKey shouldEqual orderKey; e.previousState shouldEqual OrderState("state2") }
    eventPipe.nextAny[OrderEvent] match { case e: OrderFinished ⇒ e.orderKey shouldEqual orderKey }
  }

  private def checkCollectedOrderEvents(orderKey: OrderKey): Unit = {
    sleep(500)  // The last event can be late???
    eventBus.dispatchEvents()  // Das letzte OrderFinished kann sonst verloren gehen.
    sleep(500)  // The last event can be late???
    coldEvents.toMap shouldEqual Map(
      "OrderStarted"   → OrderStarted(orderKey),
      "OrderFinished"  → OrderFinished(orderKey, OrderState("end")),
      "OrderSuspended" → OrderSuspended(orderKey),
      "OrderResumed"   → OrderResumed(orderKey)
    )
    hotEvents shouldEqual Map(
      "OrderStarted UnmodifiableOrder"            → OrderStarted(orderKey),
      "OrderStarted Order"                        → OrderStarted(orderKey),
      "OrderFinished UnmodifiableOrder"           → OrderFinished(orderKey, OrderState("end")),
      "OrderStepStarted UnmodifiableOrder state1" → OrderStepStarted(orderKey, OrderState("state1"), TaskId.Null),
      "OrderStepStarted Order state1"             → OrderStepStarted(orderKey, OrderState("state1"), TaskId.Null),
      "OrderStepEnded UnmodifiableOrder state1"   → OrderStepEnded(orderKey, OrderNodeTransition.Success),
      "OrderStepEnded Order state1"               → OrderStepEnded(orderKey, OrderNodeTransition.Success),
      "OrderStepStarted UnmodifiableOrder state2" → OrderStepStarted(orderKey, OrderState("state2"), TaskId.Null),
      "OrderStepStarted Order state2"             → OrderStepStarted(orderKey, OrderState("state2"), TaskId.Null),
      "OrderStepEnded UnmodifiableOrder state2"   → OrderStepEnded(orderKey, OrderNodeTransition.Success),
      "OrderStepEnded Order state2"               → OrderStepEnded(orderKey, OrderNodeTransition.Success),
      "OrderSuspended UnmodifiableOrder"          → OrderSuspended(orderKey),
      "OrderResumed UnmodifiableOrder"            → OrderResumed(orderKey))
  }

  @EventHandler def handleEvent(e: OrderStarted): Unit =
    addEvent(coldEvents, "OrderStarted" → e)

  @HotEventHandler def handleHotEvent(e: OrderStarted, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderStarted UnmodifiableOrder" → e)

  @HotEventHandler def handleHotEvent(e: OrderStarted, o: Order): Unit =
    addEvent(hotEvents, "OrderStarted Order" → e)

  @EventHandler def handleEvent(e: OrderFinished): Unit =
    addEvent(coldEvents, "OrderFinished" → e)

  @HotEventHandler def handleHotEvent(e: OrderFinished, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderFinished UnmodifiableOrder" → e)

  @HotEventHandler def handleHotEvent(e: OrderStepStarted, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, s"OrderStepStarted UnmodifiableOrder ${o.state}" → e.copy(taskId = TaskId.Null))

  @HotEventHandler def handleHotEvent(e: OrderStepStarted, o: Order): Unit =
    addEvent(hotEvents, s"OrderStepStarted Order ${o.state}" → e.copy(taskId = TaskId.Null))

  @HotEventHandler def handleHotEvent(e: OrderStepEnded, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, s"OrderStepEnded UnmodifiableOrder ${o.state}" → e)

  @HotEventHandler def handleHotEvent(e: OrderStepEnded, o: Order): Unit =
    addEvent(hotEvents, s"OrderStepEnded Order ${o.state}" → e)

  @EventHandler def handleEvent(e: OrderSuspended): Unit =
    addEvent(coldEvents, "OrderSuspended" → e)

  @HotEventHandler def handleHotEvent(e: OrderSuspended, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderSuspended UnmodifiableOrder" → e)

  @EventHandler def handleEvent(e: OrderResumed): Unit =
    addEvent(coldEvents, "OrderResumed" → e)

  @HotEventHandler def handleHotEvent(e: OrderResumed, o: UnmodifiableOrder): Unit =
    addEvent(hotEvents, "OrderResumed UnmodifiableOrder" → e)

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
