package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
import com.sos.scheduler.engine.data.event.KeyedEvent.NoKey
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent, NoKeyEvent}
import com.sos.scheduler.engine.eventbus.SchedulerEventBusTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class SchedulerEventBusTest extends FreeSpec {
  // Annotations are tested in SchedulerEventBusJavaTest

  "Subscriptions" - {
    "on NoKeyEvent" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      withCloser { implicit closer ⇒
        eventBus.on[A] {
          case KeyedEvent(NoKey, e: Ax) ⇒ a += 1
        }
        eventBus.publish(Ax())
        assertResult(0)(a)
        eventBus.dispatchEvents()
        assertResult(1)(a)

        eventBus.publish(Ay())
        eventBus.dispatchEvents()
        assertResult(1)(a)

        eventBus.publish(B())
        eventBus.dispatchEvents()
        assertResult(1)(a)
      }
      // on[] is unsubscribed now
      eventBus.publish(Ax())
      eventBus.dispatchEvents()
      assertResult(1)(a)
    }

    "on" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      withCloser { implicit closer ⇒
        eventBus.on[TestEvent] {
          case KeyedEvent(100, K1("TEST")) ⇒ a += 1
        }
        eventBus.publish(KeyedEvent(K1("TEST"))(100))
        assertResult(0)(a)
        eventBus.dispatchEvents()
        assertResult(1)(a)

        eventBus.publish(KeyedEvent(K1("OTHER"))(100))
        eventBus.publish(KeyedEvent(K1("TEST"))(0))
        eventBus.dispatchEvents()
        assertResult(1)(a)
      }
      // on[] is unsubscribed now
      eventBus.publish(KeyedEvent(K1("TEST"))(100))
      eventBus.dispatchEvents()
      assertResult(1)(a)
    }

    "onHot" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      withCloser { implicit closer ⇒
        eventBus.onHot[TestEvent] {
          case KeyedEvent(100, K1("TEST")) ⇒ a += 1
        }
        eventBus.publish(KeyedEvent(K1("TEST"))(100))
        assertResult(1)(a)

        eventBus.publish(KeyedEvent(K1("OTHER"))(100))
        eventBus.publish(KeyedEvent(K1("TEST"))(0))
        assertResult(1)(a)
      }
      // on[] is unsubscribed now
      eventBus.publish(KeyedEvent(K1("TEST"))(100))
      eventBus.dispatchEvents()
      assertResult(1)(a)
    }

    "onHotEventSourceEvent" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      var source: EventSource = null
      withCloser { implicit closer ⇒
        eventBus.onHotEventSourceEvent[TestEvent] {
          case KeyedEvent(100, EventSourceEvent(K1("TEST"), s)) ⇒
            a += 1
            source = s
          case x ⇒
            a += 100000
        }
        val src = new EventSource {}
        eventBus.publish(KeyedEvent(K1("TEST"))(100), src)
        assertResult(1)(a)
        source should be theSameInstanceAs src
      }
    }
  }
}

private object SchedulerEventBusTest {
  private sealed trait A extends NoKeyEvent
  private case class Ax() extends A
  private case class Ay() extends A
  private case class B() extends NoKeyEvent

  private sealed trait TestEvent extends Event {
    type Key = Int
  }
  private case class K1(string: String) extends TestEvent
}
