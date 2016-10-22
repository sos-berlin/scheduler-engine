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
  private val sync = new Sync

  "test" in {
    for (eventId ← 1L to 3L) {
      val a = sync.whenEventIsAvailable(eventId)
      assert(a eq sync.whenEventIsAvailable(eventId))
      assert(!a.isCompleted)
      queue.add(Snapshot(eventId, TestEvent))
      sync.onNewEvent(eventId)
      assert(a.isCompleted)
      assert(!sync.whenEventIsAvailable(eventId).isCompleted)
      assert(!sync.whenEventIsAvailable(eventId).isCompleted)
    }
  }
}

object SyncTest {
  private object TestEvent extends NoKeyEvent
}
