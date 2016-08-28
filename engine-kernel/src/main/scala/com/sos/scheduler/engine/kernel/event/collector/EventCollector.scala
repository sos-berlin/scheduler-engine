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

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class EventCollector @Inject()(
  eventBus: SchedulerEventBus)
  (implicit ec: ExecutionContext)
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
      .add(Snapshot(keyedEvent.event)(eventId))
    keyedEventQueue.add(Snapshot(keyedEvent)(eventId))
    val p = eventArrivedPromise
    //if (eventArrivedPromiseUsed) {
      eventArrivedPromise = Promise[Unit]()
    //}
    p.success(())
  }

  /**
    * @param reverse, true may be slow
    */
  def whenEvents(after: EventId, reverse: Boolean = false): Future[Iterator[Snapshot[AnyKeyedEvent]]] =
    onEventAvailable(after, events(after, reverse = reverse))

  def whenEventsForKey[E <: Event: ClassTag](key: E#Key, after: EventId, reverse: Boolean = false): Future[Iterator[Snapshot[E]]] =
    onEventAvailable(after, events(after, reverse = reverse) collect {
      case snapshot @ Snapshot(KeyedEvent(k, event: E @unchecked))
        if (implicitClass[E] isAssignableFrom event.getClass) && key == k ⇒
          Snapshot(event)(snapshot.eventId)
    })

  private def onEventAvailable[A](after: EventId, body: ⇒ A): Future[A] =
    if (keyedEventQueue.hasAfter(after))
      Future.successful(body)
    else {
      //eventArrivedPromiseUsed = true
      for (() ← eventArrivedPromise.future) yield body
    }

  private def events(after: EventId, reverse: Boolean): Iterator[Snapshot[AnyKeyedEvent]] =
    if (reverse) reverseEvents(after) else events(after)

  def events(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    keyedEventQueue.events(after)

  /** May be slow */
  private def reverseEvents(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    keyedEventQueue.reverseEvents(after)

  def lastEventId: EventId = ids.last

  def newEventId(): EventId = ids.next()
}

object EventCollector {
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
