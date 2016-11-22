package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.common.scalautil.Futures.implicits.RichFutureFuture
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.timer.TimerService
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, EventRequest, EventSeq, KeyedEvent, ReverseEventRequest, Snapshot}
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.EventCollector._
import com.typesafe.config.Config
import java.time.Instant
import java.time.Instant.now
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class EventCollector @Inject()(
  val eventIdGenerator: EventIdGenerator,
  eventBus: SchedulerEventBus,
  timerService: TimerService,
  configuration: Configuration = Configuration.Default)
  (implicit ec: ExecutionContext)
extends HasCloser {

  private[collector] val keyedEventQueue = new KeyedEventQueue(sizeLimit = configuration.queueSize)
  //private val keyToEventQueue = new concurrent.TrieMap[Any, EventQueue]()

  private val sync = new Sync(timerService)

  eventBus.onHot[Event] { case keyedEvent if isCollectableEvent(keyedEvent.event) ⇒
    //keyToEventQueue.getOrElseUpdate(keyedEvent.key, new EventQueue(EventQueueSizeLimitPerKey))
    //  .add(Snapshot(eventId, keyedEvent.event))
    val eventId = eventIdGenerator.next()
    keyedEventQueue.add(Snapshot(eventId, keyedEvent))
    sync.onNewEvent(eventId)
  }

  def when[E <: Event](
    request: EventRequest[E],
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true)
  : Future[EventSeq[Iterator, KeyedEvent[E]]]
  =
    whenAny[E](request, Set[Class[_ <: E]](request.eventClass), predicate)

  def whenAny[E <: Event](
    request: EventRequest[E],
    eventClasses: Set[Class[_ <: E]],
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true)
  : Future[EventSeq[Iterator, KeyedEvent[E]]]
  =
    whenAnyKeyedEvents(
      request,
      collect = {
        case e if eventClasses.exists(_ isAssignableFrom e.event.getClass) && predicate(e.asInstanceOf[KeyedEvent[E]]) ⇒
          e.asInstanceOf[KeyedEvent[E]]
      })

  def whenForKey[E <: Event](
    request: EventRequest[E],
    key: E#Key,
    predicate: E ⇒ Boolean = (_: E) ⇒ true)
  : Future[EventSeq[Iterator, E]]
  =
    whenAnyKeyedEvents(
      request,
      collect = {
        case e if (request.eventClass isAssignableFrom e.event.getClass) && e.key == key && predicate(e.event.asInstanceOf[E]) ⇒
          e.event.asInstanceOf[E]
      })

  private def whenAnyKeyedEvents[E <: Event, A](request: EventRequest[E], collect: PartialFunction[AnyKeyedEvent, A]): Future[EventSeq[Iterator, A]] =
    whenAnyKeyedEvents2(request.after, now + (request.timeout min MaxTimeout), collect, request.limit)

  private def whenAnyKeyedEvents2[A](after: EventId, until: Instant, collect: PartialFunction[AnyKeyedEvent, A], limit: Int): Future[EventSeq[Iterator, A]] =
    (for (_ ← sync.whenEventIsAvailable(after, until)) yield
      collectEventsSince(after, collect, limit) match {
        case o @ EventSeq.NonEmpty(_) ⇒
          Future.successful(o)
        case EventSeq.Empty(lastEventId) if now < until ⇒
          whenAnyKeyedEvents2(lastEventId, until, collect, limit)
        case EventSeq.Empty(lastEventId) ⇒
          Future.successful(EventSeq.Empty(lastEventId))
        case EventSeq.Torn ⇒
          Future.successful(EventSeq.Torn)
    }).flatten

  private def collectEventsSince[A](after: EventId, collect: PartialFunction[AnyKeyedEvent, A], limit: Int): EventSeq[Iterator, A] = {
    require(limit >= 0, "limit must be >= 0")
    val eventSnapshotOption = keyedEventQueue.after(after)
    var lastEventId = after
    eventSnapshotOption match {
      case Some(eventSnapshots) ⇒
        val eventIterator =
          eventSnapshots
            .map { o ⇒ lastEventId = o.eventId; o }
            .collect {
              case snapshot if collect.isDefinedAt(snapshot.value) ⇒
                Snapshot(snapshot.eventId, collect(snapshot.value))
            }
            .take(limit)
        if (eventIterator.nonEmpty)
          EventSeq.NonEmpty(eventIterator)
        else
          EventSeq.Empty(lastEventId)
      case None ⇒
        EventSeq.Torn
    }
  }

  def reverse[E <: Event](
    request: ReverseEventRequest[E],
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true)
  : Iterator[Snapshot[KeyedEvent[E]]]
  =
    keyedEventQueue.reverseEvents(after = request.after)
      .collect {
        case snapshot if request.eventClass isAssignableFrom snapshot.value.event.getClass ⇒
          snapshot.asInstanceOf[Snapshot[KeyedEvent[E]]]
      }
      .filter(snapshot ⇒ predicate(snapshot.value))
      .take(request.limit)

  def reverseForKey[E <: Event](
    request: ReverseEventRequest[E],
    key: E#Key,
    predicate: E ⇒ Boolean = (_: E) ⇒ true)
  : Iterator[Snapshot[E]]
  =
    keyedEventQueue.reverseEvents(after = request.after)
      .collect {
        case snapshot if request.eventClass isAssignableFrom snapshot.value.event.getClass ⇒
          snapshot.asInstanceOf[Snapshot[KeyedEvent[E]]]
      }
      .collect {
        case Snapshot(eventId, KeyedEvent(`key`, event)) if predicate(event) ⇒
          Snapshot(eventId, event)
      }
      .take(request.limit)

  def newSnapshot[A](a: A) = eventIdGenerator.newSnapshot(a)
}

object EventCollector {
  private val MaxTimeout = 1.h  // Limits open requests, and avoids arithmetic overflow
  final case class Configuration(queueSize: Int)

  object Configuration {
    val Default = Configuration(queueSize = 10000)

    def fromSubConfig(config: Config) = Configuration(
      queueSize = config.getInt("queue-size")
    )
  }

  private def isCollectableEvent(event: Event): Boolean =
    event match {
      //case _: InfoOrHigherLogged ⇒ true
      case _: Logged ⇒ false  // We don't want the flood of Logged events
      case _ ⇒ true
    }
}
