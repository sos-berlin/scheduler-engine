package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.data.queries.JobChainNodeQuery
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import com.sos.scheduler.engine.kernel.order.DirectOrderClient
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class JocOrderStatisticsChangedSourceInjectible @Inject private(
  protected val eventCollector: EventCollector,
  protected implicit val executionContext: ExecutionContext,
  orderClient: DirectOrderClient)
extends JocOrderStatisticsChangedSource {
  protected def jocOrderStatistics(query: JobChainNodeQuery) = orderClient.jocOrderStatistics(query)
}
