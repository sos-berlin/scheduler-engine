package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.common.event.EventIdGenerator
import com.sos.scheduler.engine.common.event.collector.{EventCollector, EventIdGenerating}
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.timer.TimerService
import com.sos.scheduler.engine.data.event.{Event, EventId}
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.SchedulerEventCollector._
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class SchedulerEventCollector @Inject()(
  configuration: EventCollector.Configuration,
  protected val eventIdGenerator: EventIdGenerator)
  (implicit
    timerService: TimerService,
    protected val eventBus: SchedulerEventBus,
    protected val executionContext: ExecutionContext
)
extends EventCollector(initialOldestEventId = EventId.BeforeFirst, configuration)(timerService, executionContext)
with HasCloser
with EventIdGenerating
{
  eventBus.onHot[Event] {
    case e if isCollectableEvent(e.event) ⇒ putEvent(e)
  }
}

object SchedulerEventCollector {

  private def isCollectableEvent(event: Event): Boolean =
    event match {
      //case _: InfoOrHigherLogged ⇒ true
      case _: Logged ⇒ false  // We don't want the flood of Logged events
      case _ ⇒ true
    }

}
