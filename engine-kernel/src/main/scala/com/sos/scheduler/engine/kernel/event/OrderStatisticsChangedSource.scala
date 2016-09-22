package com.sos.scheduler.engine.kernel.event

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.event.{Snapshot, _}
import com.sos.scheduler.engine.data.filebased.FileBasedEvent
import com.sos.scheduler.engine.data.order.{OrderEvent, OrderKey, OrderStatistics, OrderStatisticsChanged}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, PathQuery}
import com.sos.scheduler.engine.kernel.event.OrderStatisticsChangedSource._
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import scala.PartialFunction.cond
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
@ImplementedBy(classOf[OrderStatisticsChangedSourceInjectible])
trait OrderStatisticsChangedSource
extends HasCloser {

  protected def eventCollector: EventCollector
  protected def orderStatistics(query: JobChainQuery): Future[Snapshot[OrderStatistics]]
  protected implicit def executionContext: ExecutionContext

  def whenOrderStatisticsChanged(after: EventId, query: PathQuery = PathQuery.All): Future[Snapshot[OrderStatisticsChanged]] =
    for (_ ← eventCollector.whenAny[Event](Set(classOf[OrderEvent], classOf[FileBasedEvent]), after = after, pathPredicate(query));
         snapshot ← orderStatistics(JobChainQuery(query)))
      yield snapshot map OrderStatisticsChanged.apply
}

object OrderStatisticsChangedSource {
  private def pathPredicate(query: PathQuery)(e: KeyedEvent[Event]) =
    cond(e) {
      case KeyedEvent(key: OrderKey, _) ⇒ query matches key.jobChainPath
    }
}
