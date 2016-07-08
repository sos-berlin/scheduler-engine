package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.compounds.OrdersFullOverview
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.collection.immutable
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait SchedulerClient extends CommandClient {

  def overview: Future[SchedulerOverview]

  final def orderOverviews: Future[immutable.Seq[OrderOverview]] =
    orderOverviews(OrderQuery.All)

  def orderOverviews(query: OrderQuery = OrderQuery.All): Future[immutable.Seq[OrderOverview]]

  final def ordersFullOverview: Future[OrdersFullOverview] = ordersFullOverview(OrderQuery.All)

  def ordersFullOverview(query: OrderQuery = OrderQuery.All): Future[OrdersFullOverview]
}
