package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{Event, EventId, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.OrderView
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import scala.collection.immutable.Seq
import scala.concurrent.Future
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
trait SchedulerClient extends SchedulerOverviewClient with CommandClient with FileBasedClient with OrderClient {

  def jobChainOverview(jobChainPath: JobChainPath): Future[Snapshot[JobChainOverview]]

  def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[Seq[JobChainOverview]]]

  def jobChainDetailed(jobChainPath: JobChainPath): Future[Snapshot[JobChainDetailed]]

//  def jobOverviews: Future[Snapshot[Seq[JobOverview]]]
//
//  def jobOverview(jobPath: JobPath): Future[Snapshot[JobOverview]]
//
//  def processClassOverviews: Future[Snapshot[Seq[ProcessClassOverview]]]
//
//  def processClassOverview(processClassPath: ProcessClassPath): Future[Snapshot[ProcessClassOverview]]
//
//  def taskOverview(taskId: TaskId): Future[Snapshot[TaskOverview]]

  def events[E <: Event: ClassTag](after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[KeyedEvent[E]]]]]

  final def ordersComplemented[V <: OrderView: OrderView.Companion]: Future[Snapshot[OrdersComplemented[V]]] =
    ordersComplementedBy[V](OrderQuery.All)

  final def orderTreeComplemented[V <: OrderView: OrderView.Companion]: Future[Snapshot[OrderTreeComplemented[V]]] =
    orderTreeComplementedBy[V](OrderQuery.All)

  final def jobChainOverviews: Future[Snapshot[Seq[JobChainOverview]]] =
    jobChainOverviewsBy(JobChainQuery.All)
}
