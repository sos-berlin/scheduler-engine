package com.sos.scheduler.engine.client.api

import com.sos.jobscheduler.data.event.Snapshot
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderKey, OrderView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, OrderQuery}
import scala.collection.immutable
import scala.collection.immutable.Seq
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait OrderClient {

  def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey): Future[Snapshot[V]]

  def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[immutable.Seq[V]]]

  def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[OrdersComplemented[V]]]

  def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[OrderTreeComplemented[V]]]

  final def orders[V <: OrderView: OrderView.Companion]: Future[Snapshot[Seq[V]]] =
    ordersBy(OrderQuery.All)

  def jocOrderStatistics(query: JobChainNodeQuery): Future[Snapshot[JocOrderStatistics]]
}
