package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.scheduler.engine.data.event.{Event, EventId, KeyedEvent}
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

  "events" in {
    autoClosing(new EventCollector(eventIdGenerator, eventBus)) { eventCollector ⇒
      assert(eventCollector.events(after = EventId.BeforeFirst).isEmpty)
      eventBus.publish(KeyedEvent(A1)("1"))
      eventBus.publish(KeyedEvent(A1)("2"))
      val snapshots = eventCollector.events(after = EventId.BeforeFirst).toVector
      assert((snapshots map { _.value }) == Vector(KeyedEvent(A1)("1"), KeyedEvent(A1)("2")))
      assert((eventCollector.events(after = snapshots(0).eventId).toVector map { _.value }) == Vector(KeyedEvent(A1)("2")))
      assert((eventCollector.events(after = snapshots(1).eventId).toVector map { _.value }).isEmpty)
    }
  }

  "when" in {
    autoClosing(new EventCollector(eventIdGenerator, eventBus)) { eventCollector ⇒
      val anyFuture = eventCollector.when[Event](after = EventId.BeforeFirst)
      val bFuture = eventCollector.when[BEvent](after = EventId.BeforeFirst)
      assert(!anyFuture.isCompleted)
      eventBus.publish(KeyedEvent(A1)("1"))
      assert(((anyFuture await 100.ms).toList map { _.value }) == List(KeyedEvent(A1)("1")))

      assert(!bFuture.isCompleted)
      eventBus.publish(KeyedEvent(B1)("2"))
      assert(((bFuture await 100.ms).toList map { _.value }) == List(KeyedEvent(B1)("2")))
    }
  }

  "whenForKey" in {
    autoClosing(new EventCollector(eventIdGenerator, eventBus)) { eventCollector ⇒
      eventBus.publish(KeyedEvent(A1)("1"))
      eventBus.publish(KeyedEvent(B1)("1"))
      eventBus.publish(KeyedEvent(A2)("1"))
      eventBus.publish(KeyedEvent(A2)("2"))
      eventBus.publish(KeyedEvent(B2)("1"))
      def eventsForKey[E <: Event: ClassTag](key: E#Key) =
        (eventCollector.whenForKey[E](key, after = EventId.BeforeFirst) await 1.s).toVector map { _.value }
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
