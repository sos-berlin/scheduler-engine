package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.kernel.event.DirectEventClient._
import com.sos.scheduler.engine.kernel.event.collector.{EventCollector, EventIdGenerator}
import scala.collection.immutable.Seq
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
trait DirectEventClient {
  protected def eventCollector: EventCollector
  protected def eventIdGenerator: EventIdGenerator
  protected implicit def executionContext: ExecutionContext

  def events[E <: Event](request: EventRequest[E]): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    events[E](request, (_: KeyedEvent[E]) ⇒ true)

  def events[E <: Event](request: EventRequest[E], predicate: KeyedEvent[E] ⇒ Boolean): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    wrapIntoSnapshot(
      eventCollector.when[E](
        request,
        keyedEvent ⇒ eventIsSelected(keyedEvent.event) && KeyedEventJsonFormat.canSerialize(keyedEvent)))

  def eventsReverse[E <: Event](request: ReverseEventRequest[E]): Future[Snapshot[Seq[Snapshot[KeyedEvent[E]]]]] =
    Future.successful(eventIdGenerator.newSnapshot(
      eventCollector.reverse[E](
          request,
          keyedEvent ⇒ eventIsSelected(keyedEvent.event) && KeyedEventJsonFormat.canSerialize(keyedEvent))
        .toVector))

  private def eventIsSelected(event: Event): Boolean =
    event match {
      //case _: InfoOrHigherLogged ⇒ true
      case _: Logged ⇒ false
      case _ ⇒ true
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
}

object DirectEventClient {
  private val KeyedEventJsonFormat = implicitly[KeyedTypedEventJsonFormat[Event]]
}
