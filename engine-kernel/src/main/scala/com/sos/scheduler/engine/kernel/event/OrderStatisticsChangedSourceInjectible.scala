package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.data.queries.JobChainQuery
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import com.sos.scheduler.engine.kernel.order.DirectOrderClient
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class OrderStatisticsChangedSourceInjectible @Inject private(
  protected val eventCollector: EventCollector,
  protected implicit val executionContext: ExecutionContext,
  orderClient: DirectOrderClient)
extends OrderStatisticsChangedSource {
  protected def orderStatistics = orderClient.orderStatistics(JobChainQuery.All)
}
