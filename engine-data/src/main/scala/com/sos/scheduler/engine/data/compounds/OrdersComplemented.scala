package com.sos.scheduler.engine.data.compounds

import com.sos.scheduler.engine.data.job.{JobOverview, TaskOverview}
import com.sos.scheduler.engine.data.jobchain.{JobChainOverview, JobNodeOverview}
import com.sos.scheduler.engine.data.order.OrderView
import com.sos.scheduler.engine.data.processclass.ProcessClassOverview
import scala.collection.immutable.Seq
import spray.json.DefaultJsonProtocol._
import spray.json.RootJsonFormat

/**
  * @author Joacim Zschimmer
  */
final case class OrdersComplemented[V <: OrderView](
  orders: Seq[V],
  usedJobChains: Seq[JobChainOverview],
  usedNodes: Seq[JobNodeOverview],
  usedJobs: Seq[JobOverview],
  usedTasks: Seq[TaskOverview],
  usedProcessClasses: Seq[ProcessClassOverview])

object OrdersComplemented {
  implicit def jsonFormat[V <: OrderView: OrderView.Companion: RootJsonFormat] = jsonFormat6(OrdersComplemented[V])
}
