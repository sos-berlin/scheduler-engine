package com.sos.scheduler.engine.eventbus

import com.google.common.io.Closer
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.event.Event
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
    "on" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      autoClosing(Closer.create()) { implicit closer ⇒
        eventBus.on[A] {
          case e: A1 ⇒ a += 1
        }
        eventBus.publish(A1())
        assertResult(0)(a)
        eventBus.dispatchEvents()
        assertResult(1)(a)

        eventBus.publish(A2())
        eventBus.dispatchEvents()
        assertResult(1)(a)

        eventBus.publish(B())
        eventBus.dispatchEvents()
        assertResult(1)(a)
      }
      // on[] is unsubscribed now
      eventBus.publish(A1())
      eventBus.dispatchEvents()
      assertResult(1)(a)
    }

    "onHot" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      autoClosing(Closer.create()) { implicit closer ⇒
        eventBus.onHot[A] {
          case e: A1 ⇒ a += 1
        }
        eventBus.publish(A1())
        assertResult(1)(a)

        eventBus.publish(A2())
        assertResult(1)(a)

        eventBus.publish(B())
        assertResult(1)(a)
      }
      // on[] is unsubscribed now
      eventBus.publish(A1())
      eventBus.dispatchEvents()
      assertResult(1)(a)
    }

    "onHotEventSourceEvent" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      var source: EventSource = null
      autoClosing(Closer.create()) { implicit closer ⇒
        eventBus.onHotEventSourceEvent[A] {
          case EventSourceEvent(e: A1, s) ⇒
            a += 1
            source = s
        }
        val src = new EventSource {}
        eventBus.publish(A1(), src)
        assertResult(1)(a)
        source should be theSameInstanceAs src
      }
    }
  }
}

private object SchedulerEventBusTest {
  private trait A extends Event
  private case class A1() extends A
  private case class A2() extends A
  private case class B() extends Event
}
