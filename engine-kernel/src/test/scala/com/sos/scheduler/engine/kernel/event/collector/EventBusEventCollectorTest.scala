package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.common.event.EventIdGenerator
import com.sos.scheduler.engine.common.event.collector.EventCollector
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.timer.TimerService
import com.sos.scheduler.engine.data.event.{Event, EventRequest, EventSeq, KeyedEvent}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.EventBusEventCollectorTest._
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.ExecutionContext.Implicits.global

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class EventBusEventCollectorTest extends FreeSpec with BeforeAndAfterAll {

  private val eventBus = new SchedulerEventBus
  private val eventIdGenerator = new EventIdGenerator
  private lazy val timerService = TimerService()

  override protected def afterAll() = {
    timerService.close()
    super.afterAll()
  }

  "test" in {
    autoClosing(new EventBusEventCollector(eventIdGenerator, timerService, eventBus, EventCollector.Configuration.ForTest)) { eventCollector ⇒
      val aEventId = eventIdGenerator.lastUsedEventId
      eventCollector.when[AEvent.type](EventRequest(aEventId, timeout = 0.s)) await 1.s shouldEqual
        EventSeq.Empty(aEventId)
      val event = KeyedEvent(AEvent)("1")
      eventBus.publish(event)
      eventCollector.when[AEvent.type](EventRequest(aEventId, timeout = 0.s)) await 1.s match {
        case EventSeq.NonEmpty(snapshots) ⇒
          assert((snapshots.toList map { _.value }) == List(event))
        case _ ⇒ fail()
      }
    }
  }
}

object EventBusEventCollectorTest {
  private object AEvent extends Event {
    type Key = String
  }
}
