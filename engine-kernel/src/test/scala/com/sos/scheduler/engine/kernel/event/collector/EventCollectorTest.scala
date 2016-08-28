package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, KeyedEvent, Snapshot}
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

  "events" in {
    autoClosing(new EventCollector(eventBus)) { eventCollector ⇒
      assert(eventCollector.events(after = EventId.BeforeFirst).isEmpty)
      eventBus.publish(KeyedEvent(A1)("1"))
      eventBus.publish(KeyedEvent(A1)("2"))
      val snapshots = eventCollector.events(after = EventId.BeforeFirst).toVector
      assert((snapshots map { _.value }) == Vector(KeyedEvent(A1)("1"), KeyedEvent(A1)("2")))
      assert((eventCollector.events(after = snapshots(0).eventId).toVector map { _.value }) == Vector(KeyedEvent(A1)("2")))
      assert((eventCollector.events(after = snapshots(1).eventId).toVector map { _.value }).isEmpty)
    }
  }

  "whenEvents" in {
    autoClosing(new EventCollector(eventBus)) { eventCollector ⇒
      val future = eventCollector.whenEvents(after = EventId.BeforeFirst)
      assert(!future.isCompleted)
      eventBus.publish(KeyedEvent(A1)("1"))
      val iterator: Iterator[Snapshot[AnyKeyedEvent]] = future await 100.ms
      assert(iterator.toList ==
        List(Snapshot(KeyedEvent(A1)("1"))(UncheckedEventId)))
    }
  }

  "whenEventsForKey" in {
    autoClosing(new EventCollector(eventBus)) { eventCollector ⇒
      eventBus.publish(KeyedEvent(A1)("1"))
      eventBus.publish(KeyedEvent(B1)("1"))
      eventBus.publish(KeyedEvent(A2)("1"))
      eventBus.publish(KeyedEvent(A2)("2"))
      eventBus.publish(KeyedEvent(B2)("1"))
      def eventsForKey[E <: Event: ClassTag](key: E#Key) =
        eventCollector.whenEventsForKey[E](key, after = EventId.BeforeFirst).successValue.toVector map { _.value }
      assert(eventsForKey[AEvent]("1") == Vector(A1, A2))
      assert(eventsForKey[AEvent]("2") == Vector(A2))
      assert(eventsForKey[BEvent]("1") == Vector(B1, B2))
    }
  }
}

object EventCollectorTest {
  private val UncheckedEventId = EventId(999999999)
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
