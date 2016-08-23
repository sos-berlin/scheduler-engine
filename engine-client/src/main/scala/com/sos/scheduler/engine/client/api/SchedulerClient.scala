package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, EventId, Snapshot}
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

  def overview: Future[Snapshot[SchedulerOverview]]

  def orderOverviewsBy(query: OrderQuery): Future[Snapshot[Seq[OrderOverview]]]

  def ordersComplementedBy(query: OrderQuery): Future[Snapshot[OrdersComplemented]]

  def orderTreeComplementedBy(query: OrderQuery): Future[Snapshot[OrderTreeComplemented]]

  def jobChainOverview(jobChainPath: JobChainPath): Future[Snapshot[JobChainOverview]]

  def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[Seq[JobChainOverview]]]

  def jobChainDetails(jobChainPath: JobChainPath): Future[Snapshot[JobChainDetails]]

//  def jobOverviews: Future[Snapshot[Seq[JobOverview]]]
//
//  def jobOverview(jobPath: JobPath): Future[Snapshot[JobOverview]]
//
//  def processClassOverviews: Future[Snapshot[Seq[ProcessClassOverview]]]
//
//  def processClassOverview(processClassPath: ProcessClassPath): Future[Snapshot[ProcessClassOverview]]
//
//  def taskOverview(taskId: TaskId): Future[Snapshot[TaskOverview]]

  def events(after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[AnyKeyedEvent]]]]

  final def orderOverviews: Future[Snapshot[Seq[OrderOverview]]] =
    orderOverviewsBy(OrderQuery.All)

  final def ordersComplemented: Future[Snapshot[OrdersComplemented]] =
    ordersComplementedBy(OrderQuery.All)

  final def orderTreeComplemented: Future[Snapshot[OrderTreeComplemented]] =
    orderTreeComplementedBy(OrderQuery.All)

  final def jobChainOverviews: Future[Snapshot[Seq[JobChainOverview]]] =
    jobChainOverviewsBy(JobChainQuery.All)
}
