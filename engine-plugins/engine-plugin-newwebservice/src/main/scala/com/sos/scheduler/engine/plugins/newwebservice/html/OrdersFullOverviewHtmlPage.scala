package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersFullOverview
import com.sos.scheduler.engine.data.job.JobOverview
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderOverviewCollection}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersFullOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}
import scala.xml.Utility.trim

/**
  * @author Joacim Zschimmer
  */
final class OrdersFullOverviewHtmlPage private(
  fullOverview: OrdersFullOverview,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  private val taskMap = fullOverview.usedTasks toKeyedMap { _.id }
  private val jobMap = fullOverview.usedJobs toKeyedMap { _.path }
  private val collection = OrderOverviewCollection(fullOverview.orders)
  import collection._

  protected def title = "Orders"

  def node = page(ordersStatistics ++ orderTable(orderOverviews))

  private def ordersStatistics =
    <p style="margin-bottom: 30px">{
      s"$size orders: $inProcessCount in process using ${jobMap.size} jobs, $suspendedCount suspended, $blacklistedCount blacklisted"
    }</p>

  private def orderTable(orders: immutable.Seq[OrderOverview]) = trim(
    <table class="table table-condensed table-hover">
      <thead>
        <th>JobChain</th>
        <th>OrderId</th>
        <th>SourceType</th>
        <th>Task / Date</th>
        <th>Flags</th>
        <th>FileBasedState</th>
      </thead>
      <tbody>{
        for (order ← orders.sorted) yield {
          import order._
          val taskNode: Option[xml.Node] = for (t ← taskId) yield {
            val job: Option[JobOverview] = taskMap.get(t) flatMap { task ⇒ jobMap.get(task.job) }
            <b>{"Task " + ((job map { _.path.string }) ++ Some(t.number)).mkString(":")}</b>  // "JobPath:TaskId"
          }
          def nextStepEntry = nextStepAt map instantWithDurationToHtml
          <tr class={orderToTrClass(order)}>
            <td>{orderKey.jobChainPath.string}</td>
            <td>{orderKey.id.string}</td>
            <td>{sourceType}</td>
            <td>{(taskNode orElse nextStepEntry).orNull}</td>
            <td>{(isSuspended option "suspended") ++ (isBlacklisted option "blacklisted") mkString " "}</td>
            <td>{fileBasedStateToHtml(fileBasedState)}</td>
          </tr>
        }}</tbody>
    </table>)
}

object OrdersFullOverviewHtmlPage {

  import scala.language.implicitConversions

  implicit def apply(fullOverview: OrdersFullOverview)(implicit client: DirectSchedulerClient, ec: ExecutionContext): Future[HtmlPage] =
    for (schedulerOverview ← client.overview) yield new OrdersFullOverviewHtmlPage(fullOverview, schedulerOverview)

  private def orderToTrClass(order: OrderOverview) =
    if (order.isSuspended || order.isBlacklisted) "warning"
    else if (order.taskId.isDefined) "info"
    else if (!order.fileBasedState.isOkay) "danger"
    else null
}
