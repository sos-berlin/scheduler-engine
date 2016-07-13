package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersFullOverview
import com.sos.scheduler.engine.data.folder.{FolderPath, FolderTree}
import com.sos.scheduler.engine.data.job.JobOverview
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainQuery}
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderQuery, OrderSourceType}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersFullOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}
import scalatags.Text.all._
import scalatags.Text.attrs
import scalatags.text.Frag

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

  protected def title = "Orders"

  override protected val css = s"""
h3 {
  margin-top: 30px;
}
div.jobChainHeadline {
  font-weight: bold;
  margin: 0 0 5px 0;
}
div.nodeHeadline {
  padding: 5px 0 0 5px;
  font-weight: bold;
}
div.nodeOrders {
  background-color: #f5f5f5;
  border-radius: 2px;
}
div.orderSelection {
  float: right;
  line-height: 1em;
  background-color: #f5f5f5;
  padding: 2px 4px;
  border-radius: 2px;
}
""" + super.css

  def scalatag = htmlPage(
    raw(s"<script type='text/javascript'>${orderSelection.javascript}</script>"),
    orderSelection.html,
    headline,
    ordersStatistics,
    query.jobChainQuery.reduce match {
      case jobChainPath: JobChainPath ⇒ div(jobChainOrders(jobChainPath, fullOverview.orders))
      case folderPath: FolderPath ⇒ div(treeHtml(FolderTree(folderPath, fullOverview.orders)))
      case _ ⇒ div(treeHtml(FolderTree(FolderPath.Root, fullOverview.orders)))
    })


  private def ordersStatistics = {
    val statistics = new OrderOverview.Statistics(fullOverview.orders)
    import statistics.{blacklistedCount, count, inProcessCount, suspendedCount}
    p(
      s"$count orders: $inProcessCount in process using ${jobMap.size} jobs, $suspendedCount suspended, $blacklistedCount blacklisted")
  }

  def treeHtml(tree: FolderTree[OrderOverview]): Vector[Frag] =
    Vector[Frag](h3("Folder ", folderPathToOrdersA(tree.path)(tree.path.string))) ++
    folderOrders(tree.leafs map { _.obj }) ++
    (for (folder ← tree.subfolders.sorted; o ← treeHtml(folder)) yield o)

  private def folderOrders(orders: immutable.Seq[OrderOverview]): immutable.Iterable[Frag] =
    for ((jobChainPath, orders) ← orders groupBy { _.orderKey.jobChainPath };
         o ← jobChainOrders(jobChainPath, orders))
      yield o

  private def jobChainOrders(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]) =
      Vector(div(cls := "jobChainHeadline")(
        s"JobChain ",
        jobChainPathToOrdersA(jobChainPath)(jobChainPath.string),
        span(paddingLeft := 10.px)(" "),
        jobChainPathToA(jobChainPath)("(definition)"))) ++
      nodeOrders(orders)

  private def nodeOrders(orders: immutable.Seq[OrderOverview]): immutable.Iterable[Frag] =
    for ((node, orders) ← orders groupBy { _.orderState }) yield
      div(cls := "nodeOrders")(
        div(cls := "nodeHeadline")(s"Node ${node.string}"),
        orderTable(orders)
      )

  private def orderTable(orders: immutable.Seq[OrderOverview]): Frag =
    table(cls := "table table-condensed table-hover")(
      thead(
        th("OrderId"),
        th(span(cls := "visible-lg-block")("SourceType"),
        th("Task / Date"),
        th("Flags"),
        th(small("FileBasedState"))),
      tbody((orders.par map orderToTr).seq)))

  private def orderToTr(order: OrderOverview) = {
    //import order._  // does not work with ScalaTags ?
    val taskOption = for (t ← order.taskId) yield {
      val job: Option[JobOverview] = taskMap.get(t) flatMap { task ⇒ jobMap.get(task.job) }
      b(span(cls := "visible-lg-inline")("Task ", ((job map { _.path.string }) ++ Some(t.number)).mkString(":"))) :: Nil // "JobPath:TaskId"
    }
    def nextStepEntry = order.nextStepAt map instantWithDurationToHtml
    tr(cls := orderToTrClass(order))(
      td(order.orderKey.id.string),
      td(span(cls := "visible-lg-block")(order.sourceType.toString)),
      td(taskOption orElse nextStepEntry getOrElse EmptyFrag :: Nil: _*),
      td((order.isSuspended option "suspended") ++ (order.isBlacklisted option "blacklisted") mkString " "),
      td(if (order.sourceType == OrderSourceType.fileBased) small(fileBasedStateToHtml(order.fileBasedState)) else EmptyFrag))
  }

  private def folderPathToOrdersA(path: FolderPath) = queryToA(query.copy(jobChainQuery = JobChainQuery(path)))

  private def jobChainPathToOrdersA(path: JobChainPath) = queryToA(query.copy(jobChainQuery = JobChainQuery(path)))

  private def queryToA(query: OrderQuery) = a(href := uris.order(query, returnType = None))

  private def jobChainPathToA(path: JobChainPath) = a(href := uris.jobChain.details(path))

  private object orderSelection {
    def html =
      div(cls := "orderSelection")(
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
