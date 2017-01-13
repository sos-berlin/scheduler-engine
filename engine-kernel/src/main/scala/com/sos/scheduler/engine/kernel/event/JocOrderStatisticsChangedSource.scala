package com.sos.scheduler.engine.kernel.event

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.event.collector.EventCollector
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, JocOrderStatisticsChanged, OrderEvent, OrderKey}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, PathQuery}
import com.sos.scheduler.engine.kernel.event.JocOrderStatisticsChangedSource._
import java.time.Duration
import scala.PartialFunction.cond
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
@ImplementedBy(classOf[JocOrderStatisticsChangedSourceInjectible])
trait JocOrderStatisticsChangedSource
extends HasCloser {

  protected def eventCollector: EventCollector
  protected def jocOrderStatistics(query: JobChainNodeQuery): Future[Snapshot[JocOrderStatistics]]
  protected implicit def executionContext: ExecutionContext

  def whenJocOrderStatisticsChanged(after: EventId, timeout: Duration, query: PathQuery = PathQuery.All): Future[Snapshot[JocOrderStatisticsChanged]] =
    for (eventSeq ← eventCollector.when(EventRequest.singleClass[OrderEvent](after = after, timeout), pathPredicate(query));
         snapshot ← jocOrderStatistics(JobChainNodeQuery(JobChainQuery(query, isDistributed = Some(false/*No database access*/)))))
      yield snapshot map JocOrderStatisticsChanged.apply
}

object JocOrderStatisticsChangedSource {
  private def pathPredicate(query: PathQuery)(e: KeyedEvent[Event]) =
    cond(e) {
      case KeyedEvent(key: OrderKey, _) ⇒ query matches key.jobChainPath
    }
}
