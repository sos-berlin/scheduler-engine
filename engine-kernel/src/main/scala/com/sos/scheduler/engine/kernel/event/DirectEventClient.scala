package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.kernel.event.DirectEventClient._
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import java.time.Duration
import scala.collection.immutable.Seq
import scala.concurrent.{ExecutionContext, Future}
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
trait DirectEventClient {
  protected def eventCollector: EventCollector
  protected implicit def executionContext: ExecutionContext

  def events[E <: Event: ClassTag](after: EventId, timeout: Duration, limit: Int = Int.MaxValue): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    wrapIntoSnapshot(
      eventCollector.when[E](
        after,
        timeout,
        keyedEvent ⇒ eventIsSelected(keyedEvent.event) && KeyedEventJsonFormat.canSerialize(keyedEvent),
        limit = limit))

  def eventsReverse[E <: Event: ClassTag](after: EventId = EventId.BeforeFirst, limit: Int): Future[Snapshot[Seq[Snapshot[KeyedEvent[E]]]]] =
    Future.successful(eventCollector.newSnapshot(
      eventCollector.reverse[E](
          after = after,
          keyedEvent ⇒ eventIsSelected(keyedEvent.event) && KeyedEventJsonFormat.canSerialize(keyedEvent),
          limit = limit)
        .toVector))

  private def eventIsSelected(event: Event): Boolean =
    event match {
      //case _: InfoOrHigherLogged ⇒ true
      case _: Logged ⇒ false
      case _ ⇒ true
    }

  def eventsForKey[E <: Event: ClassTag](key: E#Key, after: EventId, timeout: Duration, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[EventSeq[Seq, E]]] =
    wrapIntoSnapshot(
      eventCollector.whenForKey(
        key,
        after,
        timeout,
        eventTypedJsonFormat.canSerialize,
        limit = limit,
        reverse = reverse))

  private def wrapIntoSnapshot[E](future: Future[EventSeq[Iterator, E]]): Future[Snapshot[EventSeq[Seq, E]]] =
    for (eventSeq ← future) yield
      eventCollector.newSnapshot(eventSeq match {
        case EventSeq.NonEmpty(events) ⇒ EventSeq.NonEmpty(events.toVector)
        case o: EventSeq.Empty ⇒ o
        case EventSeq.Torn ⇒ EventSeq.Torn
      })
}

object DirectEventClient {
  private val KeyedEventJsonFormat = implicitly[KeyedTypedEventJsonFormat[Event]]
}
