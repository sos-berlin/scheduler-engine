package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, EventId, Snapshot}
import java.util.NoSuchElementException
import scala.collection.AbstractIterator
import scala.collection.JavaConversions._

/**
  * @author Joacim Zschimmer
  */
final class KeyedEventQueue(sizeLimit: Int) {
  private val queue = new java.util.concurrent.ConcurrentSkipListMap[java.lang.Long, Snapshot[AnyKeyedEvent]]
  private var queueSize: Int = 0
  @volatile
  private var lastRemovedFirstId: EventId = 0

  def add(snapshot: Snapshot[AnyKeyedEvent]): Unit = {
    if (queueSize >= sizeLimit) {
      lastRemovedFirstId = queue.firstKey
      queue.remove(lastRemovedFirstId)
      queueSize -= 1
      // TODO Cancel iterator if not after queue.firstEvent
    }
    queue.put(snapshot.eventId, snapshot)
    queueSize += 1
  }

  def hasAfter(after: EventId) = queue.navigableKeySet.higher(after) != null

  def after(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    queue.navigableKeySet.tailSet(after, false).iterator map { eventId ⇒
      //if (after != 0 && after < queue.firstKey) ... events lost
      //if (after > lastRemovedEventId) ... TODO cancel iterator if queue.remove() has been called
      val value = queue.get(eventId)
      if (value == null) throw new NoSuchElementException("Event queue overflow while reading")
      value
  }

  def reverseEvents(after: EventId): Iterator[Snapshot[AnyKeyedEvent]] =
    new AbstractIterator[Snapshot[AnyKeyedEvent]] {
      private val bufferedDelegate = (queue.navigableKeySet.descendingIterator takeWhile { _ > after } map queue.get).buffered
      def hasNext = bufferedDelegate.hasNext && bufferedDelegate.head != null
      def next() = {
        val o = bufferedDelegate.next
        if (o == null) throw new NoSuchElementException
        o
      }
    }
}
