package com.sos.scheduler.engine.client.api

import com.sos.jobscheduler.data.event.{Event, EventSeq, KeyedEvent, Stamped, SomeEventRequest}
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.job.{JobPath, JobView}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.OrderView
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
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

  def jobChainOverview(jobChainPath: JobChainPath): Future[Stamped[JobChainOverview]]

  def jobChainOverviewsBy(query: JobChainQuery): Future[Stamped[Seq[JobChainOverview]]]

  def jobChainDetailed(jobChainPath: JobChainPath): Future[Stamped[JobChainDetailed]]

  def job[V <: JobView: JobView.Companion](path: JobPath): Future[Stamped[V]]

  def jobs[V <: JobView: JobView.Companion](query: PathQuery): Future[Stamped[Seq[V]]]

  def events[E <: Event](request: SomeEventRequest[E]): Future[Stamped[EventSeq[Seq, KeyedEvent[E]]]]

  def eventsByPath[E <: Event](request: SomeEventRequest[E], query: PathQuery): Future[Stamped[EventSeq[Seq, KeyedEvent[E]]]]

  final def ordersComplemented[V <: OrderView: OrderView.Companion]: Future[Stamped[OrdersComplemented[V]]] =
    ordersComplementedBy[V](OrderQuery.All)

  final def orderTreeComplemented[V <: OrderView: OrderView.Companion]: Future[Stamped[OrderTreeComplemented[V]]] =
    orderTreeComplementedBy[V](OrderQuery.All)

  final def jobChainOverviews: Future[Stamped[Seq[JobChainOverview]]] =
    jobChainOverviewsBy(JobChainQuery.All)
}
