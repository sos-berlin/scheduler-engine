package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
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
      withCloser { implicit closer ⇒
        eventBus.on[A] {
          case e: Ax ⇒ a += 1
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

    "onHot" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      withCloser { implicit closer ⇒
        eventBus.onHot[A] {
          case e: Ax ⇒ a += 1
        }
        eventBus.publish(Ax())
        assertResult(1)(a)

        eventBus.publish(Ay())
        assertResult(1)(a)

        eventBus.publish(B())
        assertResult(1)(a)
      }
      // on[] is unsubscribed now
      eventBus.publish(Ax())
      eventBus.dispatchEvents()
      assertResult(1)(a)
    }

    "onHotEventSourceEvent" in {
      val eventBus = new SchedulerEventBus
      var a = 0
      var source: EventSource = null
      withCloser { implicit closer ⇒
        eventBus.onHotEventSourceEvent[A] {
          case EventSourceEvent(e: Ax, s) ⇒
            a += 1
            source = s
        }
        val src = new EventSource {}
        eventBus.publish(Ax(), src)
        assertResult(1)(a)
        source should be theSameInstanceAs src
      }
    }
  }
}

private object SchedulerEventBusTest {
  private trait A extends Event
  private case class Ax() extends A
  private case class Ay() extends A
  private case class B() extends Event
}
