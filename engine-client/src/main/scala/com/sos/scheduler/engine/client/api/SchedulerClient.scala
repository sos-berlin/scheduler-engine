package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented, SchedulerResponse}
import com.sos.scheduler.engine.data.event.{IdAndEvent, _}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetails, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.OrderOverview
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.collection.immutable.Seq
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait SchedulerClient extends CommandClient {

  def overview: Future[SchedulerResponse[SchedulerOverview]]

  def orderOverviewsBy(query: OrderQuery): Future[SchedulerResponse[Seq[OrderOverview]]]

  def ordersComplementedBy(query: OrderQuery): Future[SchedulerResponse[OrdersComplemented]]

  def orderTreeComplementedBy(query: OrderQuery): Future[SchedulerResponse[OrderTreeComplemented]]

  def jobChainOverview(jobChainPath: JobChainPath): Future[SchedulerResponse[JobChainOverview]]

  def jobChainOverviewsBy(query: JobChainQuery): Future[SchedulerResponse[Seq[JobChainOverview]]]

  def jobChainDetails(jobChainPath: JobChainPath): Future[SchedulerResponse[JobChainDetails]]

//  def jobOverviews: Future[SchedulerResponse[Seq[JobOverview]]]
//
//  def jobOverview(jobPath: JobPath): Future[SchedulerResponse[JobOverview]]
//
//  def processClassOverviews: Future[SchedulerResponse[Seq[ProcessClassOverview]]]
//
//  def processClassOverview(processClassPath: ProcessClassPath): Future[SchedulerResponse[ProcessClassOverview]]
//
//  def taskOverview(taskId: TaskId): Future[SchedulerResponse[TaskOverview]]

  def events(after: EventId): Future[SchedulerResponse[Seq[IdAndEvent]]]

  final def orderOverviews: Future[SchedulerResponse[Seq[OrderOverview]]] =
    orderOverviewsBy(OrderQuery.All)

  final def ordersComplemented: Future[SchedulerResponse[OrdersComplemented]] =
    ordersComplementedBy(OrderQuery.All)

  final def orderTreeComplemented: Future[SchedulerResponse[OrderTreeComplemented]] =
    orderTreeComplementedBy(OrderQuery.All)

  final def jobChainOverviews: Future[SchedulerResponse[Seq[JobChainOverview]]] =
    jobChainOverviewsBy(JobChainQuery.All)
}
