package com.sos.scheduler.engine.kernel.event.collector

import com.sos.jobscheduler.common.event.EventIdGenerator
import com.sos.jobscheduler.common.event.collector.EventCollector
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.timer.TimerService
import com.sos.jobscheduler.data.event.{Event, EventRequest, EventSeq, KeyedEvent}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.SchedulerEventCollectorTest._
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.ExecutionContext.Implicits.global

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SchedulerEventCollectorTest extends FreeSpec with BeforeAndAfterAll {

  private val eventIdGenerator = new EventIdGenerator
  private implicit val eventBus = new SchedulerEventBus
  private implicit lazy val timerService = TimerService()

  override protected def afterAll() = {
    timerService.close()
    super.afterAll()
  }

  "test" in {
    autoClosing(new SchedulerEventCollector(EventCollector.Configuration.ForTest, eventIdGenerator))
    { eventCollector ⇒
      val aEventId = eventIdGenerator.lastUsedEventId
      eventCollector.when(EventRequest.singleClass[AEvent.type](aEventId, timeout = 0.s)) await 1.s shouldEqual
        EventSeq.Empty(aEventId)
      val event = KeyedEvent(AEvent)("1")
      eventBus.publish(event)
      eventCollector.when(EventRequest.singleClass[AEvent.type](aEventId, timeout = 0.s)) await 1.s match {
        case EventSeq.NonEmpty(snapshots) ⇒
          assert((snapshots.toList map { _.value }) == List(event))
        case _ ⇒ fail()
      }
    }
  }
}

object SchedulerEventCollectorTest {
  private object AEvent extends Event {
    type Key = String
  }
}
