package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetails, JobChainOverview, JobChainPath, JobChainQuery}
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.collection.immutable
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait SchedulerClient extends CommandClient {

  def overview: Future[SchedulerOverview]

  def orderOverviewsBy(query: OrderQuery): Future[immutable.Seq[OrderOverview]]

  def ordersComplementedBy(query: OrderQuery): Future[OrdersComplemented]

  def orderTreeComplementedBy(query: OrderQuery): Future[OrderTreeComplemented]

  def jobChainOverview(jobChainPath: JobChainPath): Future[JobChainOverview]

  def jobChainOverviewsBy(query: JobChainQuery): Future[immutable.Seq[JobChainOverview]]

  def jobChainDetails(jobChainPath: JobChainPath): Future[JobChainDetails]


  final def orderOverviews: Future[immutable.Seq[OrderOverview]] =
    orderOverviewsBy(OrderQuery.All)

  final def ordersComplemented: Future[OrdersComplemented] =
    ordersComplementedBy(OrderQuery.All)

  final def orderTreeComplemented: Future[OrderTreeComplemented] =
    orderTreeComplementedBy(OrderQuery.All)

  final def jobChainOverviews: Future[immutable.Seq[JobChainOverview]] =
    jobChainOverviewsBy(JobChainQuery.All)
}
