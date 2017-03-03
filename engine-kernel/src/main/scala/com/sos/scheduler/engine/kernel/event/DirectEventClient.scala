package com.sos.scheduler.engine.kernel.event

import com.sos.jobscheduler.common.event.EventIdGenerator
import com.sos.jobscheduler.common.event.collector.EventCollector
import com.sos.jobscheduler.data.event._
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
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

  def events[E <: Event](request: SomeEventRequest[E]): Future[Stamped[TearableEventSeq[Seq, KeyedEvent[E]]]] =
    eventsByPredicate[E](request, _ ⇒ true)

  def eventsByPath[E <: Event](request: SomeEventRequest[E], query: PathQuery): Future[Stamped[TearableEventSeq[Seq, KeyedEvent[E]]]] =
    eventsByPredicate[E](
      request,
      predicate = {
        case KeyedEvent(key: TypedPath, _) ⇒ query matchesAnyType key
        case _ ⇒ false
      })

  def eventsByPredicate[E <: Event](request: SomeEventRequest[E], predicate: KeyedEvent[E] ⇒ Boolean): Future[Stamped[TearableEventSeq[Seq, KeyedEvent[E]]]] =
    eventIdGenerator.stampTearableEventSeq {
      eventCollector.byPredicate[E](
        request,
        keyedEvent ⇒ KeyedEventJsonFormat.canSerialize(keyedEvent) && predicate(keyedEvent))
    }

  def eventsByKeyAndPredicate[E <: Event](request: EventRequest[E], key: E#Key): Future[Stamped[TearableEventSeq[Seq, E]]] =
    eventIdGenerator.stampTearableEventSeq {
      eventCollector.byKeyAndPredicate(request, key, eventTypedJsonFormat.canSerialize)
    }

  def eventsForKey[E <: Event](request: EventRequest[E], key: E#Key): Future[Stamped[TearableEventSeq[Seq, E]]] =
    eventIdGenerator.stampTearableEventSeq {
      eventCollector.whenForKey(request, key, eventTypedJsonFormat.canSerialize)
    }

  def eventsReverseForKey[E <: Event](request: ReverseEventRequest[E], key: E#Key): Future[Stamped[Seq[Stamped[E]]]] =
    Future.successful(eventIdGenerator.stamp(
      eventCollector.reverseForKey(request, key, eventTypedJsonFormat.canSerialize)
      .toVector))
}

object DirectEventClient {
  private val KeyedEventJsonFormat = implicitly[KeyedTypedEventJsonFormat[Event]]
}
