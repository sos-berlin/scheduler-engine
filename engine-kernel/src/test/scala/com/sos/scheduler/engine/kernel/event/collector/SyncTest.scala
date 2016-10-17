package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.data.event.{EventId, NoKeyEvent, Snapshot}
import com.sos.scheduler.engine.kernel.event.collector.SyncTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SyncTest extends FreeSpec {

  private val queue = new KeyedEventQueue(sizeLimit = 100)
  private val sync = new Sync {
    def hasAfter(eventId: EventId) = queue.hasAfter(eventId)
  }

  "test" in {
    for (eventId ← 1L to 3L) {
      val a = sync.whenEventIsAvailable(eventId)
      val b = sync.whenEventIsAvailable(eventId)
      assert(a eq b)
      assert(!a.isCompleted)
      queue.add(Snapshot(eventId, TestEvent))
      sync.onNewEvent()
      assert(a.isCompleted)
    }
  }
}

object SyncTest {
  private object TestEvent extends NoKeyEvent
}
