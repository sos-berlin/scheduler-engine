package com.sos.scheduler.engine.tests.schedulertest

import com.google.common.io.Closer
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.jobscheduler.data.event.KeyedEvent.NoKey
import com.sos.scheduler.engine.data.scheduler.{SchedulerClosed, SchedulerEvent, SchedulerInitiated, SchedulerState, SchedulerStateChanged, SchedulerTerminatedEvent}
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
  private val receivedEvents = mutable.Buffer[SchedulerEvent]()

  eventBus.onHot[SchedulerEvent] {
    case KeyedEvent(NoKey, e) ⇒ receivedEvents += e
  }

  "SchedulerEvent" in {
    assert(receivedEvents == List(
      SchedulerInitiated,
      SchedulerStateChanged(SchedulerState.starting),
      SchedulerStateChanged(SchedulerState.running)))
  }

  "controller.terminate, .close()" in {
    eventBus.awaiting[SchedulerClosed.type](NoKey) {
      eventBus.awaiting[SchedulerTerminatedEvent](NoKey) {
        controller.terminateScheduler()
        controller.close()
      }
    }
  }
}
