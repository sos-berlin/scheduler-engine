package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.common.event.EventIdGenerator
import com.sos.scheduler.engine.common.event.collector.EventCollector
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.kernel.event.DirectEventClient._
import scala.collection.immutable.Seq
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
trait DirectEventClient {
  protected def eventCollector: EventCollector
  protected def eventIdGenerator: EventIdGenerator
  protected implicit def executionContext: ExecutionContext

  def events[E <: Event](request: SomeEventRequest[E]): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    eventsByPredicate[E](request, _ ⇒ true)

  def eventsByPath[E <: Event](request: SomeEventRequest[E], query: PathQuery): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    eventsByPredicate[E](
      request,
      predicate = {
        case KeyedEvent(key: TypedPath, _) ⇒ query matchesAnyType key
        case _ ⇒ false
      })

  def eventsByPredicate[E <: Event](request: SomeEventRequest[E], predicate: KeyedEvent[E] ⇒ Boolean): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    request match {
      case request: EventRequest[E] ⇒
        wrapIntoSnapshot(
          eventCollector.when[E](
            request,
            keyedEvent ⇒ KeyedEventJsonFormat.canSerialize(keyedEvent) && predicate(keyedEvent)))
      case request: ReverseEventRequest[E] ⇒
        reverseEventIteratorToEventSeqSnapshot(
          eventCollector.reverse[E](
            request,
            keyedEvent ⇒ KeyedEventJsonFormat.canSerialize(keyedEvent) && predicate(keyedEvent)))
  }

  def eventsForKey[E <: Event](request: EventRequest[E], key: E#Key): Future[Snapshot[EventSeq[Seq, E]]] =
    wrapIntoSnapshot(eventCollector.whenForKey(request, key, eventTypedJsonFormat.canSerialize))

  def eventsReverseForKey[E <: Event](request: ReverseEventRequest[E], key: E#Key): Future[Snapshot[Seq[Snapshot[E]]]] =
    Future.successful(eventIdGenerator.newSnapshot(
      eventCollector.reverseForKey(request, key, eventTypedJsonFormat.canSerialize)
      .toVector))

  private def wrapIntoSnapshot[E](future: ⇒ Future[EventSeq[Iterator, E]]): Future[Snapshot[EventSeq[Seq, E]]] = {
    val eventId = eventIdGenerator.next()
    for (eventSeq ← future) yield
      eventSeq match {
        case EventSeq.NonEmpty(iterator) ⇒
          val events = iterator.toVector
          Snapshot(math.max(eventId, events.last.eventId), EventSeq.NonEmpty(events))
        case o: EventSeq.Empty ⇒
          eventIdGenerator.newSnapshot(o)
        case EventSeq.Torn ⇒
          eventIdGenerator.newSnapshot(EventSeq.Torn)
      }
  }

  private def reverseEventIteratorToEventSeqSnapshot[E <: Event](iterator: Iterator[Snapshot[KeyedEvent[E]]]): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    wrapIntoSnapshot(
      Future.successful(
        if (iterator.nonEmpty)
          EventSeq.NonEmpty(iterator)
        else
          EventSeq.Empty(eventIdGenerator.lastUsedEventId)))  // eventReverse is only for testing purposes
}

object DirectEventClient {
  private val KeyedEventJsonFormat = implicitly[KeyedTypedEventJsonFormat[Event]]
}
