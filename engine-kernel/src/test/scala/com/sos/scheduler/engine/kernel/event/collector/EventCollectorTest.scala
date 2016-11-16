package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.scheduler.engine.data.event.{Event, EventId, EventRequest, EventSeq, KeyedEvent}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.EventCollectorTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class EventCollectorTest extends FreeSpec {

  private val eventBus = new SchedulerEventBus
  private val eventIdGenerator = new EventIdGenerator

  "eventCollector.after" in {
    autoClosing(new EventCollector(eventIdGenerator, eventBus)) { eventCollector ⇒
      import eventCollector.keyedEventQueue
      assert(keyedEventQueue.after(after = EventId.BeforeFirst).get.isEmpty)
      eventBus.publish(KeyedEvent(A1)("1"))
      eventBus.publish(KeyedEvent(A1)("2"))
      val snapshots = keyedEventQueue.after(after = EventId.BeforeFirst).get.toVector
      assert((snapshots map { _.value }) == Vector(KeyedEvent(A1)("1"), KeyedEvent(A1)("2")))
      assert((keyedEventQueue.after(after = snapshots(0).eventId).get.toVector map { _.value }) == Vector(KeyedEvent(A1)("2")))
      assert((keyedEventQueue.after(after = snapshots(1).eventId).get.toVector map { _.value }).isEmpty)
    }
  }

  "eventCollector.when with torn event stream" in {
    autoClosing(new EventCollector(eventIdGenerator, eventBus, EventCollector.Configuration(queueSize = 2))) { eventCollector ⇒
      val anyFuture = eventCollector.when(EventRequest[Event](after = EventId.BeforeFirst, 30.s))
      val bFuture = eventCollector.when(EventRequest[BEvent](after = EventId.BeforeFirst, 30.s))
      assert(!anyFuture.isCompleted)
      eventBus.publish(KeyedEvent(A1)("1"))
      val EventSeq.NonEmpty(anyEvents) = anyFuture await 100.ms
      assert((anyEvents.toList map { _.value }) == List(KeyedEvent(A1)("1")))

      assert(!bFuture.isCompleted)
      eventBus.publish(KeyedEvent(B1)("2"))
      val EventSeq.NonEmpty(bEventsIterator) = bFuture await 100.ms
      val bEvents = bEventsIterator.toVector
      assert((bEvents map { _.value }) == Vector(KeyedEvent(B1)("2")))

      // Third event, overflowing the queue
      eventBus.publish(KeyedEvent(B1)("2"))

      val EventSeq.NonEmpty(cEventIterator) = eventCollector.when(EventRequest[BEvent](after = bEvents.last.eventId, 1.s)) await 100.ms
      assert((cEventIterator.toList map { _.value }) == List(KeyedEvent(B1)("2")))

      assert((eventCollector.when(EventRequest[BEvent](after = EventId.BeforeFirst, 1.s)) await 100.ms) == EventSeq.Torn)
    }
  }

  "eventCollector.whenForKey" in {
    autoClosing(new EventCollector(eventIdGenerator, eventBus)) { eventCollector ⇒
      eventBus.publish(KeyedEvent(A1)("1"))
      eventBus.publish(KeyedEvent(B1)("1"))
      eventBus.publish(KeyedEvent(A2)("1"))
      eventBus.publish(KeyedEvent(A2)("2"))
      eventBus.publish(KeyedEvent(B2)("1"))
      def eventsForKey[E <: Event: ClassTag](key: E#Key) = {
        val EventSeq.NonEmpty(eventIterator) = eventCollector.whenForKey[E](EventRequest(after = EventId.BeforeFirst, 20.s), key) await 10.s
        eventIterator.toVector map { _.value }
      }
      assert(eventsForKey[AEvent]("1") == Vector(A1, A2))
      assert(eventsForKey[AEvent]("2") == Vector(A2))
      assert(eventsForKey[BEvent]("1") == Vector(B1, B2))
    }
  }
}

object EventCollectorTest {
  intelliJuseImports(global)

  private trait AEvent extends Event {
    type Key = String
  }

  private case object A1 extends AEvent
  private case object A2 extends AEvent

  private trait BEvent extends Event {
    type Key = String
  }

  private case object B1 extends BEvent
  private case object B2 extends BEvent
}
