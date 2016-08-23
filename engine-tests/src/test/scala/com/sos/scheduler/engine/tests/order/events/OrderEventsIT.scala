package com.sos.scheduler.engine.tests.order.events

import com.sos.scheduler.engine.common.scalautil.Collections.implicits.InsertableMutableMap
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.eventbus.EventSourceEvent
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
  private val hotEvents = mutable.Map[String, AnyKeyedEvent]()
  private val coldEvents = mutable.Map[String, AnyKeyedEvent]()

  "Permanent order" in {
    checkOrderStates(PersistentOrderKey)
    eventPipe.nextAny[OrderNodeChanged] match {
      case KeyedEvent(orderKey, e: OrderNodeChanged) ⇒
        orderKey shouldEqual PersistentOrderKey
        assert(e.fromNodeId == NodeId("end"))
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
    eventPipe.nextAny[OrderStarted.type].key shouldEqual orderKey
    eventPipe.nextAny[OrderSuspended.type].key shouldEqual orderKey
    eventPipe.nextAny[OrderNodeChanged] match { case KeyedEvent(k, e: OrderNodeChanged) ⇒ k shouldEqual orderKey; e.fromNodeId shouldEqual NodeId("state1") }
    scheduler.executeXml(<remove_order job_chain={TestJobChainPath.string} order={orderKey.id.toString}/>)
    //eventPipe.next[KeyedEvent[OrderEvent]] match { case e: OrderFinished ⇒ e.getKey shouldEqual orderKey }
  }

  private def checkOrderStates(orderKey: OrderKey): Unit = {
    expectOrderEventsAndResumeOrder(orderKey)
    checkCollectedOrderEvents(orderKey)
  }

  private def expectOrderEventsAndResumeOrder(orderKey: OrderKey): Unit = {
    eventPipe.nextAny[OrderStarted.type] match { case KeyedEvent(k, e: OrderStarted.type) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderStepStarted] match { case KeyedEvent(k, e: OrderStepStarted) ⇒ k shouldEqual orderKey; e.nodeId shouldEqual NodeId("state1") }
    eventPipe.nextAny[OrderStepEnded] match { case KeyedEvent(k, e: OrderStepEnded) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderSuspended.type] match { case KeyedEvent(k, e: OrderSuspended.type) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderNodeChanged] match { case KeyedEvent(k, e: OrderNodeChanged) ⇒ k shouldEqual orderKey; e.fromNodeId shouldEqual NodeId("state1") }
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
    eventPipe.nextAny[OrderResumed.type] match { case KeyedEvent(k, e: OrderResumed.type) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderStepStarted] match { case KeyedEvent(k, e: OrderStepStarted) ⇒ k shouldEqual orderKey; e.nodeId shouldEqual NodeId("state2") }
    eventPipe.nextAny[OrderStepEnded] match { case KeyedEvent(k, e: OrderStepEnded) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderNodeChanged] match { case KeyedEvent(k, e: OrderNodeChanged) ⇒ k shouldEqual orderKey; e.fromNodeId shouldEqual NodeId("state2") }
    eventPipe.nextAny[OrderFinished] match { case KeyedEvent(k, e: OrderFinished) ⇒ k shouldEqual orderKey }
  }

  private def checkCollectedOrderEvents(orderKey: OrderKey): Unit = {
    sleep(500)  // The last event can be late???
    eventBus.dispatchEvents()  // Das letzte OrderFinished kann sonst verloren gehen.
    sleep(500)  // The last event can be late???
    coldEvents.toMap shouldEqual Map(
      "OrderStarted"   → KeyedEvent[OrderEvent](orderKey, OrderStarted),
      "OrderFinished"  → KeyedEvent[OrderEvent](orderKey, OrderFinished(NodeId("end"))),
      "OrderSuspended" → KeyedEvent[OrderEvent](orderKey, OrderSuspended),
      "OrderResumed"   → KeyedEvent[OrderEvent](orderKey, OrderResumed))
    hotEvents shouldEqual Map(
      "OrderStarted UnmodifiableOrder"            → KeyedEvent[OrderEvent](orderKey, OrderStarted),
      "OrderStarted Order"                        → KeyedEvent[OrderEvent](orderKey, OrderStarted),
      "OrderFinished UnmodifiableOrder"           → KeyedEvent[OrderEvent](orderKey, OrderFinished(NodeId("end"))),
      "OrderStepStarted UnmodifiableOrder state1" → KeyedEvent[OrderEvent](orderKey, OrderStepStarted(NodeId("state1"), TaskId.Null)),
      "OrderStepStarted Order state1"             → KeyedEvent[OrderEvent](orderKey, OrderStepStarted(NodeId("state1"), TaskId.Null)),
      "OrderStepEnded UnmodifiableOrder state1"   → KeyedEvent[OrderEvent](orderKey, OrderStepEnded(OrderNodeTransition.Success)),
      "OrderStepEnded Order state1"               → KeyedEvent[OrderEvent](orderKey, OrderStepEnded(OrderNodeTransition.Success)),
      "OrderStepStarted UnmodifiableOrder state2" → KeyedEvent[OrderEvent](orderKey, OrderStepStarted(NodeId("state2"), TaskId.Null)),
      "OrderStepStarted Order state2"             → KeyedEvent[OrderEvent](orderKey, OrderStepStarted(NodeId("state2"), TaskId.Null)),
      "OrderStepEnded UnmodifiableOrder state2"   → KeyedEvent[OrderEvent](orderKey, OrderStepEnded(OrderNodeTransition.Success)),
      "OrderStepEnded Order state2"               → KeyedEvent[OrderEvent](orderKey, OrderStepEnded(OrderNodeTransition.Success)),
      "OrderSuspended UnmodifiableOrder"          → KeyedEvent[OrderEvent](orderKey, OrderSuspended),
      "OrderResumed UnmodifiableOrder"            → KeyedEvent[OrderEvent](orderKey, OrderResumed))
  }

  controller.eventBus.on[OrderStarted.type] {
    case e @ KeyedEvent(orderKey, _) ⇒
      addEvent(coldEvents, "OrderStarted" → e)
  }

  controller.eventBus.onHotEventSourceEvent[OrderStarted.type] {
    case KeyedEvent(k, EventSourceEvent(e, o: UnmodifiableOrder)) ⇒
      addEvent(hotEvents, "OrderStarted UnmodifiableOrder" → KeyedEvent(e)(k))
  }

  controller.eventBus.onHotEventSourceEvent[OrderStarted.type] {
    case KeyedEvent(k, EventSourceEvent(e, o: Order)) ⇒
      addEvent(hotEvents, "OrderStarted Order" → KeyedEvent(e)(k))
  }

  controller.eventBus.on[OrderFinished] {
    case e @ KeyedEvent(orderKey, _) =>
      addEvent(coldEvents, "OrderFinished" → e)
  }

  controller.eventBus.onHotEventSourceEvent[OrderFinished] {
    case KeyedEvent(k, EventSourceEvent(e, o: UnmodifiableOrder)) ⇒
      addEvent(hotEvents, "OrderFinished UnmodifiableOrder" → KeyedEvent(e)(k))
  }

  controller.eventBus.onHotEventSourceEvent[OrderStepStarted] {
    case KeyedEvent(k, EventSourceEvent(e, o: UnmodifiableOrder)) ⇒
      addEvent(hotEvents, s"OrderStepStarted UnmodifiableOrder ${o.nodeId}" → KeyedEvent[OrderEvent](k, e.copy(taskId = TaskId.Null)))
  }

  controller.eventBus.onHotEventSourceEvent[OrderStepStarted] {
    case KeyedEvent(k, EventSourceEvent(e, o: Order)) ⇒
      addEvent(hotEvents, s"OrderStepStarted Order ${o.nodeId}" → KeyedEvent[OrderEvent](k, e.copy(taskId = TaskId.Null)))
  }

  controller.eventBus.onHotEventSourceEvent[OrderStepEnded] {
    case KeyedEvent(k, EventSourceEvent(e, o: UnmodifiableOrder)) ⇒
      addEvent(hotEvents, s"OrderStepEnded UnmodifiableOrder ${o.nodeId}" → KeyedEvent(e)(k))
  }

  controller.eventBus.onHotEventSourceEvent[OrderStepEnded] {
    case KeyedEvent(k, EventSourceEvent(e, o: Order)) ⇒
      addEvent(hotEvents, s"OrderStepEnded Order ${o.nodeId}" → KeyedEvent(e)(k))
  }

  controller.eventBus.on[OrderSuspended.type] {
    case e @ KeyedEvent(orderKey, _) ⇒
      addEvent(coldEvents, "OrderSuspended" → e)
  }

  controller.eventBus.onHotEventSourceEvent[OrderSuspended.type] {
    case KeyedEvent(k, EventSourceEvent(e, o: UnmodifiableOrder)) ⇒
      addEvent(hotEvents, "OrderSuspended UnmodifiableOrder" → KeyedEvent(e)(k))
  }

  controller.eventBus.on[OrderResumed.type] {
    case e @ KeyedEvent(orderKey, _) =>
      addEvent(coldEvents, "OrderResumed" → e)
  }

  controller.eventBus.onHotEventSourceEvent[OrderResumed.type] {
    case KeyedEvent(k, EventSourceEvent(e, o: UnmodifiableOrder)) ⇒
      addEvent(hotEvents, "OrderResumed UnmodifiableOrder" → KeyedEvent(e)(k))
  }

  private def addEvent(m: mutable.Map[String, AnyKeyedEvent], pair: (String, AnyKeyedEvent)): Unit =
    m.synchronized {
      m.insert(pair)
    }
}


private object OrderEventsIT {
  private val TestJobChainPath = JobChainPath("/a")
  private val PersistentOrderKey = TestJobChainPath orderKey "persistentOrder"
  private val TemporaryOrderKey = TestJobChainPath orderKey "1"
}
