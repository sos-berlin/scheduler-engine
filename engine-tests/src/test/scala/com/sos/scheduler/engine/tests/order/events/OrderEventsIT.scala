package com.sos.scheduler.engine.tests.order.events

import com.sos.scheduler.engine.common.scalautil.Collections.implicits.InsertableMutableMap
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
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
    scheduler.executeXml(<add_order job_chain={TestJobChainPath.string} id={TemporaryOrderKey.id.string}/>)
    checkOrderStates(TemporaryOrderKey)
  }

  "Removed temporary order" in {
    hotEvents.clear()
    coldEvents.clear()
    val orderKey = TemporaryOrderKey
    scheduler.executeXml(<add_order job_chain={TestJobChainPath.string} id={orderKey.id.string}/>)
    eventPipe.nextAny[OrderStarted.type].key shouldEqual orderKey
    eventPipe.nextAny[OrderSuspended.type].key shouldEqual orderKey
    eventPipe.nextAny[OrderNodeChanged] match { case KeyedEvent(k, e: OrderNodeChanged) ⇒ k shouldEqual orderKey; e.fromNodeId shouldEqual NodeId("state1") }
    scheduler.executeXml(<remove_order job_chain={TestJobChainPath.string} order={orderKey.id.string}/>)
    //eventPipe.next[KeyedEvent[OrderEvent]] match { case e: OrderFinished ⇒ e.getKey shouldEqual orderKey }
  }

  private def checkOrderStates(orderKey: OrderKey): Unit = {
    expectOrderEventsAndResumeOrder(orderKey)
    checkCollectedOrderEvents(orderKey)
  }

  private def expectOrderEventsAndResumeOrder(orderKey: OrderKey): Unit = {
    eventPipe.nextAny[OrderStarted.type] match { case KeyedEvent(k, _) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderStepStarted] match { case KeyedEvent(k, e) ⇒ k shouldEqual orderKey; e.nodeId shouldEqual NodeId("state1") }
    eventPipe.nextAny[OrderStepEnded] match { case KeyedEvent(k, _) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderSuspended.type] match { case KeyedEvent(k, _) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderNodeChanged] match { case KeyedEvent(k, e) ⇒ k shouldEqual orderKey; e.fromNodeId shouldEqual NodeId("state1") }
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
    eventPipe.nextAny[OrderResumed.type] match { case KeyedEvent(k, _) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderStepStarted] match { case KeyedEvent(k, e) ⇒ k shouldEqual orderKey; e.nodeId shouldEqual NodeId("state2") }
    eventPipe.nextAny[OrderStepEnded] match { case KeyedEvent(k, _) ⇒ k shouldEqual orderKey }
    eventPipe.nextAny[OrderNodeChanged] match { case KeyedEvent(k, e) ⇒ k shouldEqual orderKey; e.fromNodeId shouldEqual NodeId("state2") }
    eventPipe.nextAny[OrderFinished] match { case KeyedEvent(k, _) ⇒ k shouldEqual orderKey }
  }

  private def checkCollectedOrderEvents(orderKey: OrderKey): Unit = {
    sleep(500)  // The last event can be late???
    eventBus.dispatchEvents()  // Das letzte OrderFinished kann sonst verloren gehen.
    sleep(500)  // The last event can be late???
    coldEvents.toMap shouldEqual Map(
      "OrderStarted"   → KeyedEvent(OrderStarted)(orderKey),
      "OrderFinished"  → KeyedEvent(OrderFinished(NodeId("end")))(orderKey),
      "OrderSuspended" → KeyedEvent(OrderSuspended)(orderKey),
      "OrderResumed"   → KeyedEvent(OrderResumed)(orderKey))
    hotEvents shouldEqual Map(
      "OrderStarted UnmodifiableOrder"            → KeyedEvent(OrderStarted)(orderKey),
      "OrderStarted Order"                        → KeyedEvent(OrderStarted)(orderKey),
      "OrderFinished UnmodifiableOrder"           → KeyedEvent(OrderFinished(NodeId("end")))(orderKey),
      "OrderStepStarted UnmodifiableOrder state1" → KeyedEvent(OrderStepStarted(NodeId("state1"), TaskId.Null))(orderKey),
      "OrderStepStarted Order state1"             → KeyedEvent(OrderStepStarted(NodeId("state1"), TaskId.Null))(orderKey),
      "OrderStepEnded UnmodifiableOrder state1"   → KeyedEvent(OrderStepEnded(OrderNodeTransition.Success))(orderKey),
      "OrderStepEnded Order state1"               → KeyedEvent(OrderStepEnded(OrderNodeTransition.Success))(orderKey),
      "OrderStepStarted UnmodifiableOrder state2" → KeyedEvent(OrderStepStarted(NodeId("state2"), TaskId.Null))(orderKey),
      "OrderStepStarted Order state2"             → KeyedEvent(OrderStepStarted(NodeId("state2"), TaskId.Null))(orderKey),
      "OrderStepEnded UnmodifiableOrder state2"   → KeyedEvent(OrderStepEnded(OrderNodeTransition.Success))(orderKey),
      "OrderStepEnded Order state2"               → KeyedEvent(OrderStepEnded(OrderNodeTransition.Success))(orderKey),
      "OrderSuspended UnmodifiableOrder"          → KeyedEvent(OrderSuspended)(orderKey),
      "OrderResumed UnmodifiableOrder"            → KeyedEvent(OrderResumed)(orderKey))
  }

  eventBus.on[OrderStarted.type] {
    case e @ KeyedEvent(orderKey, _) ⇒
      addEvent(coldEvents, "OrderStarted" → e)
  }

  eventBus.onHot[OrderStarted.type] {
    case KeyedEvent(k, e) ⇒
      addEvent(hotEvents, "OrderStarted UnmodifiableOrder" → KeyedEvent(e)(k))
  }

  eventBus.onHot[OrderStarted.type] {
    case KeyedEvent(k, e) ⇒
      addEvent(hotEvents, "OrderStarted Order" → KeyedEvent(e)(k))
  }

  eventBus.on[OrderFinished] {
    case e @ KeyedEvent(orderKey, _) =>
      addEvent(coldEvents, "OrderFinished" → e)
  }

  eventBus.onHot[OrderFinished] {
    case KeyedEvent(k, e) ⇒
      addEvent(hotEvents, "OrderFinished UnmodifiableOrder" → KeyedEvent(e)(k))
  }

  eventBus.onHot[OrderStepStarted] {
    case KeyedEvent(orderKey, e) ⇒
      addEvent(hotEvents, s"OrderStepStarted UnmodifiableOrder ${orderOverview(orderKey).nodeId}" → KeyedEvent(e.copy(taskId = TaskId.Null))(orderKey))
  }

  eventBus.onHot[OrderStepStarted] {
    case KeyedEvent(orderKey, e) ⇒
      addEvent(hotEvents, s"OrderStepStarted Order ${orderOverview(orderKey).nodeId}" → KeyedEvent(e.copy(taskId = TaskId.Null))(orderKey))
  }

  eventBus.onHot[OrderStepEnded] {
    case KeyedEvent(orderKey, e) ⇒
      addEvent(hotEvents, s"OrderStepEnded UnmodifiableOrder ${orderOverview(orderKey).nodeId}" → KeyedEvent(e)(orderKey))
  }

  eventBus.onHot[OrderStepEnded] {
    case KeyedEvent(orderKey, e) ⇒
      addEvent(hotEvents, s"OrderStepEnded Order ${orderOverview(orderKey).nodeId}" → KeyedEvent(e)(orderKey))
  }

  eventBus.on[OrderSuspended.type] {
    case e @ KeyedEvent(orderKey, _) ⇒
      addEvent(coldEvents, "OrderSuspended" → e)
  }

  eventBus.onHot[OrderSuspended.type] {
    case KeyedEvent(k, e) ⇒
      addEvent(hotEvents, "OrderSuspended UnmodifiableOrder" → KeyedEvent(e)(k))
  }

  eventBus.on[OrderResumed.type] {
    case e @ KeyedEvent(orderKey, _) =>
      addEvent(coldEvents, "OrderResumed" → e)
  }

  eventBus.onHot[OrderResumed.type] {
    case KeyedEvent(k, e) ⇒
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
