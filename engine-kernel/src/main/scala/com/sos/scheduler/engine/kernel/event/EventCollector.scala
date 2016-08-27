package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.EventCollector._
import java.lang.System.currentTimeMillis
import java.util.NoSuchElementException
import java.util.concurrent.atomic.AtomicLong
import javax.inject.{Inject, Singleton}
import scala.annotation.tailrec
import scala.collection.AbstractIterator
import scala.collection.JavaConversions._
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

  private val queue = new java.util.concurrent.ConcurrentSkipListMap[java.lang.Long, Snapshot[AnyKeyedEvent]]
  private val ids = new UniqueTimestampedIdIterator

  @volatile
  private var queueSize: Int = 0
  @volatile
  private var lastRemovedFirstId: EventId = 0
  @volatile
  private var eventArrivedPromise = Promise[Unit]()
  @volatile
  private var eventArrivedPromiseUsed = false

  eventBus.onHot[Event] { case event ⇒
    val id = ids.next()
    if (queueSize >= EventQueueSizeLimit) {
      lastRemovedFirstId = queue.firstKey
      queue.remove(lastRemovedFirstId)
      queueSize -= 1
      // TODO Cancel iterator if not after queue.firstEvent
    }
    queue.put(id, Snapshot(event)(id))
    queueSize += 1
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
    if (queue.navigableKeySet.higher(after) != null)
      Future.successful(body)
    else {
      //eventArrivedPromiseUsed = true
      for (() ← eventArrivedPromise.future) yield body
    }

  private def events(after: EventId, reverse: Boolean): Iterator[Snapshot[AnyKeyedEvent]] =
    if (reverse) reverseEvents(after) else events(after)

  def events(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    queue.navigableKeySet.tailSet(after, false).iterator map { k ⇒
      //if (after != 0 && after < queue.firstKey) ... events lost
      //if (after > lastRemovedEventId) ... TODO cancel iterator if queue.remove() has been called
      val value = queue.get(k)
      if (value == null) throw new NoSuchElementException("Event queue overflow while reading")
      value
  }

  /** May be slow */
  private def reverseEvents(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    new AbstractIterator[Snapshot[AnyKeyedEvent]] {
      private val bufferedDelegate = (queue.navigableKeySet.descendingIterator takeWhile { _ > after } map queue.get).buffered
      def hasNext = bufferedDelegate.hasNext && bufferedDelegate.head != null
      def next() = {
        val o = bufferedDelegate.next
        if (o == null) throw new NoSuchElementException
        o
      }
    }

  def lastEventId: EventId = ids.last

  def newEventId(): EventId = ids.next()
}

object EventCollector {
  private val EventQueueSizeLimit = 10000  // Not too much, as long as the needed heap size has not been clarified

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
