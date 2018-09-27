package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{Event, EventSeq, KeyedEvent, Snapshot, SomeEventRequest}
import com.sos.scheduler.engine.data.job.{JobPath, JobView}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.OrderView
import com.sos.scheduler.engine.data.queries.{JobChainQuery, JobQuery, OrderQuery, PathQuery}
import scala.collection.immutable.Seq
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait SchedulerClient
extends SchedulerOverviewClient
with CommandClient
with FileBasedClient
with OrderClient
with ProcessClassClient {

  def jobChainOverview(jobChainPath: JobChainPath): Future[Snapshot[JobChainOverview]]

  def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[Seq[JobChainOverview]]]

  def jobChainDetailed(jobChainPath: JobChainPath): Future[Snapshot[JobChainDetailed]]

  def job[V <: JobView: JobView.Companion](path: JobPath): Future[Snapshot[V]]

  def jobs[V <: JobView: JobView.Companion](query: JobQuery): Future[Snapshot[Seq[V]]]

  def events[E <: Event](request: SomeEventRequest[E]): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]]

  def eventsByPath[E <: Event](request: SomeEventRequest[E], query: PathQuery): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]]

  final def ordersComplemented[V <: OrderView: OrderView.Companion]: Future[Snapshot[OrdersComplemented[V]]] =
    ordersComplementedBy[V](OrderQuery.All)

  final def orderTreeComplemented[V <: OrderView: OrderView.Companion]: Future[Snapshot[OrderTreeComplemented[V]]] =
    orderTreeComplementedBy[V](OrderQuery.All)

  final def jobChainOverviews: Future[Snapshot[Seq[JobChainOverview]]] =
    jobChainOverviewsBy(JobChainQuery.All)
}
