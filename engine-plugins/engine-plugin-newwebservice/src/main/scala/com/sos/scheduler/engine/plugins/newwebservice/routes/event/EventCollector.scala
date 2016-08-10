package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.event.{EventId, IdAndEvent}
import com.sos.scheduler.engine.data.order.OrderEvent
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventCollector._
import java.lang.System.currentTimeMillis
import java.util.NoSuchElementException
import java.util.concurrent.atomic.AtomicLong
import javax.inject.{Inject, Singleton}
import scala.annotation.tailrec
import scala.collection.JavaConversions._
import scala.concurrent.{ExecutionContext, Future, Promise}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class EventCollector @Inject private(eventBus: SchedulerEventBus)(implicit ec: ExecutionContext)
extends HasCloser {

  private val queue = new java.util.concurrent.ConcurrentSkipListMap[java.lang.Long, IdAndEvent]
  private val ids = new UniqueTimestampedIdIterator

  private var queueSize: Int = 0
  @volatile
  private var lastRemovedFirstId: EventId = 0
  @volatile
  private var eventArrivedPromise = Promise[Unit]()
  @volatile
  private var eventArrivedPromiseUsed = false

  eventBus.onHot[OrderEvent] { case event ⇒
    if (queueSize >= EventQueueSizeLimit) {
      lastRemovedFirstId = queue.firstKey
      queue.remove(lastRemovedFirstId)
      queueSize -= 1
      // TODO Cancel iterator if not after queue.firstEvent
    }
    val id = ids.next()
    queue.put(id, IdAndEvent(id, event))
    queueSize += 1
    val p = eventArrivedPromise
    //if (eventArrivedPromiseUsed) {
      eventArrivedPromise = Promise[Unit]()
    //}
    p.success(())
  }

  def iteratorFuture(after: EventId): Future[Iterator[IdAndEvent]] = {
    if (queue.navigableKeySet.higher(after) != null)
      Future.successful(iterator(after))
    else {
      //eventArrivedPromiseUsed = true
      for (() ← eventArrivedPromise.future) yield
        iterator(after)
    }
  }

  def iterator(after: EventId): Iterator[IdAndEvent] =
    queue.navigableKeySet.tailSet(after, false).iterator map { k ⇒
      val value = queue.get(k) match {
        case null ⇒ throw new NoSuchElementException("Event queue overflow while reading")
        case v: IdAndEvent ⇒ v
      }
      //if (after != 0 && after < queue.firstKey) ... events lost
      //if (after > lastRemovedEventId) ... TODO cancel iterator if queue.remove() has been called
      value
  }
}


object EventCollector {
  private val EventQueueSizeLimit = 10000

  private final class UniqueTimestampedIdIterator extends Iterator[EventId] {
    // JavaScript kann nur die ganzen Zahlen bis 2**53 (9.007.199.254.740.992) lückenlos darstellen, also 11 Bits weniger als ein Long.
    // 2 ** 53 = 9.007.199.254.740.992µs = 285 Jahre. Das reicht bis zum Jahr 1970 + 285 = 2255, bei einer Million Events/s
    private val lastResult = new AtomicLong(0)

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
