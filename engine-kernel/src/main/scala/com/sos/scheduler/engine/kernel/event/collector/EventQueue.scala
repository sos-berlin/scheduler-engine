package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.data.event.{Event, EventId, Snapshot}
import java.util.NoSuchElementException
import scala.collection.JavaConversions._

/**
  * @author Joacim Zschimmer
  */
final class EventQueue(sizeLimit: Int) {
  private val queue = new java.util.concurrent.ConcurrentSkipListMap[java.lang.Long, Snapshot[Event]]
  private var queueSize: Int = 0
  @volatile
  private var lastRemovedFirstId: EventId = 0

  def add(snapshot: Snapshot[Event]): Unit = {
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

  def after(after: EventId) = Option(queue.navigableKeySet.tailSet(after, false))

  def events(after: EventId): Iterator[Snapshot[Event]] =
    queue.navigableKeySet.tailSet(after, false).iterator map { eventId ⇒
      //if (after != 0 && after < queue.firstKey) ... events lost
      //if (after > lastRemovedEventId) ... TODO cancel iterator if queue.remove() has been called
      val value = queue.get(eventId)
      if (value == null) throw new NoSuchElementException("Event queue overflow while reading")
      value
  }
}
