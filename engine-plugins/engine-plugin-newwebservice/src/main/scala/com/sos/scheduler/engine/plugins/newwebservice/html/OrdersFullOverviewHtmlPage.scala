package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersFullOverview
import com.sos.scheduler.engine.data.job.JobOverview
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderOverviewCollection, OrderQuery, OrderSourceType}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersFullOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}
import scalatags.Text.all._
import scalatags.Text.attrs

/**
  * @author Joacim Zschimmer
  */
final class OrdersFullOverviewHtmlPage private(
  query: OrderQuery,
  fullOverview: OrdersFullOverview,
  protected val webServiceContext: WebServiceContext,
  protected val schedulerOverview: SchedulerOverview)(
  implicit ec: ExecutionContext)
extends SchedulerHtmlPage {

  private val taskMap = fullOverview.usedTasks toKeyedMap { _.id }
  private val jobMap = fullOverview.usedJobs toKeyedMap { _.path }
  private val collection = OrderOverviewCollection(fullOverview.orders)
  import collection._

  protected def title = "Orders"

  def scalatag = htmlPage(
    raw(s"<script type='text/javascript'>${orderSelection.javascript}</script>"),
    orderSelection.html,
    headline,
    ordersStatistics,
    orderTable(orderOverviews))

  private def ordersStatistics =
    p(marginBottom := "30px")(
      s"$size orders: $inProcessCount in process using ${jobMap.size} jobs, $suspendedCount suspended, $blacklistedCount blacklisted")

  private def orderTable(orders: immutable.Seq[OrderOverview]) =
    table(cls := "table table-condensed table-hover")(
      thead(
        th("JobChain"),
        th("Node"),
        th("OrderId"),
        th(span(cls := "visible-lg-block")("SourceType"),
        th("Task / Date"),
        th("Flags"),
        th(small("FileBasedState"))),
      tbody((orders.sorted.par map orderToTr).seq)))

  private def orderToTr(order: OrderOverview) = {
    //import order._  // does not work with ScalaTags ?
    val taskOption = for (t ← order.taskId) yield {
      val job: Option[JobOverview] = taskMap.get(t) flatMap { task ⇒ jobMap.get(task.job) }
      b(span(cls := "visible-lg-inline")("Task ", ((job map { _.path.string }) ++ Some(t.number)).mkString(":"))) :: Nil // "JobPath:TaskId"
    }
    def nextStepEntry = order.nextStepAt map instantWithDurationToHtml
    tr(cls := orderToTrClass(order))(
      td(jobChainPathToA(order.orderKey.jobChainPath)),
      td(order.orderState.string),
      td(order.orderKey.id.string),
      td(span(cls := "visible-lg-block")(order.sourceType.toString)),
      td(taskOption orElse nextStepEntry getOrElse EmptyFrag :: Nil: _*),
      td((order.isSuspended option "suspended") ++ (order.isBlacklisted option "blacklisted") mkString " "),
      td(if (order.sourceType == OrderSourceType.fileBased) small(fileBasedStateToHtml(order.fileBasedState)) else EmptyFrag))
  }

  private def jobChainPathToA(jobChainPath: JobChainPath) = a(href := uris.jobChain.details(jobChainPath))(jobChainPath.string)

  private object orderSelection {
    def html =
      div(float.right, lineHeight := "1em", background := "#f5f5f5", padding := "2px 4px", borderRadius := "4px")(
        "Filter",
        br,
        for ((key, valueOption) ← List("suspended" → query.isSuspended, "setback" → query.isSetback, "blacklisted" → query.isBlacklisted)) yield
          inputElement(key, valueOption) :: StringFrag(" ") :: inputElement(key, valueOption, checkedMeans = false) :: br :: Nil)

    private def inputElement(key: String, value: Option[Boolean], checkedMeans: Boolean = true) = {
      val name = if (checkedMeans) key else s"not-$key"
      val checked = !checkedMeans ^ (value getOrElse !checkedMeans)
      val onClick = s"javascript:reload({$key: document.getElementsByName('$name')[0].checked ? $checkedMeans : undefined})"
      label(
        input(attrs.name := name, `type` := "checkbox", checked option attrs.checked, attrs.onclick := onClick),
        if (checkedMeans) key else s"not")
    }

    def javascript = s"""
      function reload(change) {
        var query = ${toJavascript(query)};
        var key, v;
        var q = [];
        for (key in change) if (change.hasOwnProperty(key)) {
          v = change[key]
          if (typeof v == 'undefined') delete query[key]; else query[key] = v;
        }
        for (key in query) if (query.hasOwnProperty(key)) q.push(key + '=' + query[key].toString());
        var href = window.location.href.replace(/\\?.*/g, '');
        if (q.length) href += "?" + q.join('&');
        window.location.href = href;
      }"""
  }
}

object OrdersFullOverviewHtmlPage {

  import scala.language.implicitConversions

  def toHtml(query: OrderQuery)(fullOverview: OrdersFullOverview)(implicit context: WebServiceContext, client: DirectSchedulerClient, ec: ExecutionContext): Future[HtmlPage] =
    for (schedulerOverview ← client.overview) yield new OrdersFullOverviewHtmlPage(query, fullOverview, context, schedulerOverview)

  private def toJavascript(query: OrderQuery) =
    ((for (o ← query.isSuspended) yield s"suspended:$o") ++
    (for (o ← query.isSetback) yield s"setback:$o") ++
    (for (o ← query.isBlacklisted) yield s"blacklisted:$o"))
      .mkString("{", ",", "}")

  private def orderToTrClass(order: OrderOverview) =
    if (order.isSuspended || order.isBlacklisted) "warning"
    else if (order.taskId.isDefined) "info"
    else if (!order.fileBasedState.isOkay) "danger"
    else ""
}
