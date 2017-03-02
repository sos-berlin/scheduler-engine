package com.sos.scheduler.engine.client.api

import com.sos.jobscheduler.data.event.Stamped
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

  def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey): Future[Stamped[V]]

  def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[immutable.Seq[V]]]

  def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[OrdersComplemented[V]]]

  def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[OrderTreeComplemented[V]]]

  final def orders[V <: OrderView: OrderView.Companion]: Future[Stamped[Seq[V]]] =
    ordersBy(OrderQuery.All)

  def jocOrderStatistics(query: JobChainNodeQuery): Future[Stamped[JocOrderStatistics]]
}
