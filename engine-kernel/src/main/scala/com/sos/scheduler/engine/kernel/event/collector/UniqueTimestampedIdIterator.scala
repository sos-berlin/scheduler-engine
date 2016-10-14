package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.data.event.EventId
import java.lang.System._
import java.util.concurrent.atomic.AtomicLong
import scala.annotation.tailrec

/**
  * @author Joacim Zschimmer
  */
private[collector] final class UniqueTimestampedIdIterator extends Iterator[EventId] {

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
