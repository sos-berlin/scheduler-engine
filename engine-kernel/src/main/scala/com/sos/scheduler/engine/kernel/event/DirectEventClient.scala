package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.data.event.{KeyedTypedEventJsonFormat, _}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import scala.collection.immutable.Seq
import scala.concurrent.{ExecutionContext, Future}
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
trait DirectEventClient {
  protected def eventCollector: EventCollector
  protected implicit def executionContext: ExecutionContext

  def events[E <: Event: ClassTag](after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[KeyedEvent[E]]]]] = {
    val eventJsonFormat = implicitly[KeyedTypedEventJsonFormat[Event]]
    eventCollector.when[E](
      after,
      keyedEvent ⇒ eventIsSelected(keyedEvent.event) && (eventJsonFormat canSerialize keyedEvent),
      limit = limit,
      reverse = reverse)
    .map { snapshots ⇒
      eventCollector.newSnapshot(snapshots.toVector)
    }
  }

  private def eventIsSelected(event: Event): Boolean =
    event match {
      //case _: InfoOrHigherLogged ⇒ true
      case _: Logged ⇒ false
      case _ ⇒ true
    }

  def eventsForKey[E <: Event: ClassTag](key: E#Key, after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[E]]]] =
    eventCollector.whenForKey(
      key,
      after,
      eventTypedJsonFormat.canSerialize,
      limit = limit,
      reverse = reverse)
    .map { snapshots ⇒
      eventCollector.newSnapshot(snapshots.toVector)
    }
}
