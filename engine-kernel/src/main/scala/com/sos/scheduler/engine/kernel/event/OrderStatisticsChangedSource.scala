package com.sos.scheduler.engine.kernel.event

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.event.{Snapshot, _}
import com.sos.scheduler.engine.data.filebased.FileBasedEvent
import com.sos.scheduler.engine.data.order.{OrderEvent, OrderStatistics, OrderStatisticsChanged}
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
@ImplementedBy(classOf[OrderStatisticsChangedSourceInjectible])
trait OrderStatisticsChangedSource
extends HasCloser {

  protected def eventCollector: EventCollector
  protected def orderStatistics: Future[Snapshot[OrderStatistics]]
  protected implicit def executionContext: ExecutionContext

  def whenOrderStatisticsChanged(after: EventId): Future[Snapshot[OrderStatisticsChanged]] =
    for (_ ← eventCollector.whenAny[Event](Set(classOf[OrderEvent], classOf[FileBasedEvent]), after = after);
         snapshot ← orderStatistics)
      yield {
        Snapshot(OrderStatisticsChanged(snapshot.value))(snapshot.eventId)
      }
}
