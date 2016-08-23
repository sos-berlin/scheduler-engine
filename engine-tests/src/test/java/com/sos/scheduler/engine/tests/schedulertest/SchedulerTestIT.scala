package com.sos.scheduler.engine.tests.schedulertest

import com.google.common.io.Closer
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.event.KeyedEvent.NoKey
import com.sos.scheduler.engine.main.event.{MainEvent, SchedulerClosed, SchedulerReadyEvent, TerminatedEvent}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/**
  * Tests [[SchedulerTest]].
  */
@RunWith(classOf[JUnitRunner])
final class SchedulerTestIT extends FreeSpec with ScalaSchedulerTest {

  private implicit val unusedCloser = Closer.create()
  private val receivedEvents = mutable.Buffer[MainEvent]()

  controller.eventBus.on[MainEvent] {
    case KeyedEvent(NoKey, e) ⇒ receivedEvents += e
  }

  "SchedulerReadyEvent" in {
    assert(receivedEvents == List(SchedulerReadyEvent))
  }

  "controller.terminate, .close()" in {
    eventBus.awaitingKeyedEvent[SchedulerClosed.type](NoKey) {
      eventBus.awaitingKeyedEvent[TerminatedEvent](NoKey) {
        controller.terminateScheduler()
        controller.close()
      }
    }
  }
}
