package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersFullOverview
import com.sos.scheduler.engine.data.job.JobOverview
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderOverviewCollection, OrderQuery, OrderSourceType}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersFullOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
final class OrdersFullOverviewHtmlPage private(
  query: OrderQuery,
  fullOverview: OrdersFullOverview,
  protected val webServiceContext: WebServiceContext,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  private val taskMap = fullOverview.usedTasks toKeyedMap { _.id }
  private val jobMap = fullOverview.usedJobs toKeyedMap { _.path }
  private val collection = OrderOverviewCollection(fullOverview.orders)
  import collection._

  protected def title = "Orders"

  def node = page(
    <script type="text/javascript">{xml.Unparsed(orderSelection.javascript)}</script> ++
      orderSelection.html ++
      headline ++
      ordersStatistics ++
      orderTable(orderOverviews))

  private def ordersStatistics =
    <p style="margin-bottom: 30px">{
      s"$size orders: $inProcessCount in process using ${jobMap.size} jobs, $suspendedCount suspended, $blacklistedCount blacklisted"
    }</p>

  private def orderTable(orders: immutable.Seq[OrderOverview]) =
    <table class="table table-condensed table-hover">
      <thead>
        <th>JobChain</th>
        <th>Node</th>
        <th>OrderId</th>
        <th><span class="visible-lg-block">SourceType</span></th>
        <th>Task / Date</th>
        <th>Flags</th>
        <th><small>FileBasedState</small></th>
      </thead>
      <tbody>{
        for (order ← orders sortBy { o ⇒ (o.orderKey.jobChainPath, o.orderState, o.orderKey.id) }) yield {
          import order._
          val taskNode: Option[xml.Node] = for (t ← taskId) yield {
            val job: Option[JobOverview] = taskMap.get(t) flatMap { task ⇒ jobMap.get(task.job) }
            <b><span class="visible-lg-inline">Task </span>{((job map { _.path.string }) ++ Some(t.number)).mkString(":")}</b>  // "JobPath:TaskId"
          }
          def nextStepEntry = nextStepAt map instantWithDurationToHtml
          <tr class={orderToTrClass(order)}>
            <td><a href={uris.jobChain.details(orderKey.jobChainPath)}>{orderKey.jobChainPath.string}</a></td>
            <td>{orderState.string}</td>
            <td>{orderKey.id.string}</td>
            <td><span class="visible-lg-block">{sourceType}</span></td>
            <td>{(taskNode orElse nextStepEntry).orNull}</td>
            <td>{(isSuspended option "suspended") ++ (isBlacklisted option "blacklisted") mkString " "}</td>
            <td>{if (sourceType == OrderSourceType.fileBased) <small>{fileBasedStateToHtml(fileBasedState)}</small> else ""}</td>
          </tr>
        }}</tbody>
    </table>

  private object orderSelection {
    def html =
      <div style="float: right; line-height: 1em; background: #f5f5f5; padding: 2px 4px; border-radius: 4px">
        Filter<br/>{
          val inputs = for (
          (key, valueOption) ← List("suspended" → query.isSuspended, "setback" → query.isSetback, "blacklisted" → query.isBlacklisted)) yield
          inputElement(key, valueOption) ++ xml.Text(" ") ++ inputElement(key, valueOption, checkedMeans = false)
        inputs reduce { _ ++ <br/> ++ _ }
      }</div>

    private def inputElement(key: String, value: Option[Boolean], checkedMeans: Boolean = true): xml.Node = {
      val name = if (checkedMeans) key else s"not-$key"
      val checked = !checkedMeans ^ (value getOrElse !checkedMeans) option "checked"
      val onClick = s"javascript:reload({$key: document.getElementsByName('$name')[0].checked ? $checkedMeans : undefined})"
      <label>
        <input name={name} type="checkbox" checked={checked.orNull} onclick={onClick}/>
        {if (checkedMeans) key else s"not"}
      </label>
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
    else null
}
