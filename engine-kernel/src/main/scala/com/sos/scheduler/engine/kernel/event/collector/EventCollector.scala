package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.EventCollector._
import java.lang.System.currentTimeMillis
import java.util.concurrent.atomic.AtomicLong
import javax.inject.{Inject, Singleton}
import scala.annotation.tailrec
import scala.collection.concurrent
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.reflect.ClassTag
import scala.util.control.NonFatal

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class EventCollector @Inject()(eventBus: SchedulerEventBus)(implicit ec: ExecutionContext)
extends HasCloser {

  private val ids = new UniqueTimestampedIdIterator

  @volatile
  private var eventArrivedPromise = Promise[Unit]()
  @volatile
  private var eventArrivedPromiseUsed = false

  private val keyedEventQueue = new KeyedEventQueue(sizeLimit = KeyEventQueueSizeLimit)
  private val keyToEventQueue = new concurrent.TrieMap[Any, EventQueue]()

  eventBus.onHot[Event] { case keyedEvent ⇒
    val eventId = ids.next()
    keyToEventQueue.getOrElseUpdate(keyedEvent.key, new EventQueue(EventQueueSizeLimitPerKey))
      .add(Snapshot(eventId, keyedEvent.event))
    keyedEventQueue.add(Snapshot(eventId, keyedEvent))
    val p = eventArrivedPromise
    //if (eventArrivedPromiseUsed) {
      eventArrivedPromise = Promise[Unit]()
    //}
    p.success(())
  }

  /**
    * @param reverse, true may be slow
    */
  def when[E <: Event: ClassTag](after: EventId, predicate: Predicate[E] = matchesAll _, reverse: Boolean = false)
  : Future[Iterator[Snapshot[KeyedEvent[E]]]] =
    whenAny[E](Set(implicitClass[E]), after = after, predicate, reverse = reverse)

  def whenAny[E <: Event](eventClasses: Set[Class[_ <: E]], after: EventId, predicate: Predicate[E]  = matchesAll _, reverse: Boolean = false)
  : Future[Iterator[Snapshot[KeyedEvent[E]]]] = {
    def pred(e: AnyKeyedEvent) = (eventClasses exists { _ isAssignableFrom e.event.getClass }) && predicate(e.asInstanceOf[KeyedEvent[E]])
    for (snapshot ← whenAnyKeyedEvents(after, pred, reverse = reverse)) yield
      for (snapshot ← snapshot.value) yield
        snapshot map { _.asInstanceOf[KeyedEvent[E]] }
  }

  private def whenAnyKeyedEvents(after: EventId, predicate: Predicate[Event], reverse: Boolean = false): Future[Snapshot[Iterator[Snapshot[AnyKeyedEvent]]]] = {
    val promise = Promise[Snapshot[Iterator[Snapshot[AnyKeyedEvent]]]]()
    for (Snapshot(eventId, events) ← onEventAvailable(after, events(after, reverse = reverse)))
      try
        events filter {
          case Snapshot(_, keyedEvent) ⇒ predicate(keyedEvent)
        } match {
          case filteredEvents if filteredEvents.hasNext ⇒
            promise.success(Snapshot(eventId, filteredEvents))
          case _ ⇒
            // Filtering has left no event. So we wait for new events.
            promise.completeWith(whenAnyKeyedEvents(eventId, predicate))
        }
      catch {
        case NonFatal(t) ⇒ promise.failure(t)
      }
    promise.future
  }

  def whenForKey[E <: Event: ClassTag](key: E#Key, after: EventId, reverse: Boolean = false): Future[Snapshot[Iterator[Snapshot[E]]]] =
    onEventAvailable(after, events(after, reverse = reverse) collect {
      case Snapshot(eventId, KeyedEvent(k, event: E @unchecked))
        if (implicitClass[E] isAssignableFrom event.getClass) && key == k ⇒
          Snapshot(eventId, event)
    })

  private def onEventAvailable[A](after: EventId, body: ⇒ A): Future[Snapshot[A]] = {
    def result = Snapshot(newEventId(), body)
    if (keyedEventQueue.hasAfter(after))
      Future.successful(result)
    else {
      //eventArrivedPromiseUsed = true
      for (() ← eventArrivedPromise.future) yield result
    }
  }

  private def events(after: EventId, reverse: Boolean): Iterator[Snapshot[AnyKeyedEvent]] =
    if (reverse) reverseEvents(after) else events(after)

  private[collector] def events(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    keyedEventQueue.events(after)

  /** May be slow */
  private def reverseEvents(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    keyedEventQueue.reverseEvents(after)

  def lastEventId: EventId = ids.last

  def newEventId(): EventId = ids.next()
}

object EventCollector {
  type Predicate[E <: Event] = KeyedEvent[E] ⇒ Boolean

  def matchesAll[E <: Event](e: KeyedEvent[E]) = true

  private val KeyEventQueueSizeLimit = 10000
  private val EventQueueSizeLimitPerKey = 100

  private final class UniqueTimestampedIdIterator extends Iterator[EventId] {
    // JavaScript kann nur die ganzen Zahlen bis 2**53 (9.007.199.254.740.992) lückenlos darstellen, also 11 Bits weniger als ein Long.
    // 2 ** 53 = 9.007.199.254.740.992µs = 285 Jahre. Das reicht bis zum Jahr 1970 + 285 = 2255, bei einer Million Events/s
    private val lastResult = new AtomicLong(0)

    def last: EventId = lastResult.get

    def hasNext = true

    @tailrec
    def next(): EventId = {
      val nowId = currentTimeMillis * EventId.IdsPerMillisecond
      val last = lastResult.get
      val nextId = if (last < nowId) nowId else last + 1
      if (lastResult.compareAndSet(last, nextId))
        nextId
      else
        next()
    }
  }
}
