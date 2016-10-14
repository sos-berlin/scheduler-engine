package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.RichFutureFuture
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.EventCollector._
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class EventCollector @Inject()(eventIdGenerator: EventIdGenerator, eventBus: SchedulerEventBus)(implicit ec: ExecutionContext)
extends HasCloser {

  @volatile
  private var eventArrivedPromise = Promise[Unit]()
  private val keyedEventQueue = new KeyedEventQueue(sizeLimit = KeyEventQueueSizeLimit)
  //private val keyToEventQueue = new concurrent.TrieMap[Any, EventQueue]()

  eventBus.onHot[Event] { case keyedEvent ⇒
    //keyToEventQueue.getOrElseUpdate(keyedEvent.key, new EventQueue(EventQueueSizeLimitPerKey))
    //  .add(Snapshot(eventId, keyedEvent.event))
    keyedEventQueue.add(eventIdGenerator.newSnapshot(keyedEvent))
    val p = eventArrivedPromise
    eventArrivedPromise = Promise[Unit]()     // Renew only when needed ???
    p.success(())
  }

  /**
    * @param reverse, true may be slow
    */
  def when[E <: Event: ClassTag](
    after: EventId,
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true,
    limit: Int = Int.MaxValue,
    reverse: Boolean = false)
  : Future[Iterator[Snapshot[KeyedEvent[E]]]] =
    whenAny[E](Set(implicitClass[E]), after, predicate, limit, reverse)

  def whenAny[E <: Event](
    eventClasses: Set[Class[_ <: E]],
    after: EventId,
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true,
    limit: Int = Int.MaxValue,
    reverse: Boolean = false)
  : Future[Iterator[Snapshot[KeyedEvent[E]]]] =
    whenAnyKeyedEvents(
      after,
      collect = {
        case e if eventClasses.exists(_ isAssignableFrom e.event.getClass) && predicate(e.asInstanceOf[KeyedEvent[E]]) ⇒
          e.asInstanceOf[KeyedEvent[E]]
      },
      limit,
      reverse)

  def whenForKey[E <: Event: ClassTag](
    key: E#Key,
    after: EventId,
    predicate: E ⇒ Boolean = (_: E) ⇒ true,
    limit: Int = Int.MaxValue,
    reverse: Boolean = false)
  : Future[Iterator[Snapshot[E]]] =
    whenAnyKeyedEvents(
      after,
      collect = {
        case e if (implicitClass[E] isAssignableFrom e.event.getClass) && e.key == key && predicate(e.event.asInstanceOf[E]) ⇒
          e.event.asInstanceOf[E]
      },
      limit,
      reverse)

  private def whenAnyKeyedEvents[A](after: EventId, collect: PartialFunction[AnyKeyedEvent, A], limit: Int, reverse: Boolean): Future[Iterator[Snapshot[A]]] =
    (for (() ← whenEventIsAvailable(after)) yield {
      val eventSnapshots = events(after, reverse = reverse)
      var lastEventId = after
      val filtered =
        eventSnapshots
          .map { o ⇒ lastEventId = o.eventId; o }
          .collect {
            case snapshot if collect.isDefinedAt(snapshot.value) ⇒ Snapshot(snapshot.eventId, collect(snapshot.value))
          }
          .take(limit)
      if (filtered.nonEmpty || reverse)
        Future.successful(filtered)
      else
        whenAnyKeyedEvents(lastEventId, collect, limit, reverse)  // Filtering has left no event. So we wait for new events.
    }).flatten

  private def whenEventIsAvailable(after: EventId): Future[Unit] =
    if (keyedEventQueue.hasAfter(after))
      Future.successful(())
    else
      eventArrivedPromise.future

  private def events(after: EventId, reverse: Boolean): Iterator[Snapshot[AnyKeyedEvent]] =
    if (reverse) reverseEvents(after) else events(after)

  private[collector] def events(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    keyedEventQueue.after(after)

  /** May be slow */
  private def reverseEvents(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    keyedEventQueue.reverseEvents(after)

  def newSnapshot[A](a: A) = eventIdGenerator.newSnapshot(a)

  def lastEventId: EventId = eventIdGenerator.last
}

object EventCollector {
  private val KeyEventQueueSizeLimit = 10000
}
