package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersComplemented
import com.sos.scheduler.engine.data.folder.{FolderPath, FolderTree}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, TaskId, TaskOverview}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}
import scalatags.Text.all._
import scalatags.text.Frag

/**
  * @author Joacim Zschimmer
  */
final class OrdersHtmlPage private(
  query: OrderQuery,
  ordersComplemented: OrdersComplemented,
  protected val webServiceContext: WebServiceContext,
  protected val schedulerOverview: SchedulerOverview)(
  implicit ec: ExecutionContext)
extends SchedulerHtmlPage {

  private val taskIdToOverview: Map[TaskId, TaskOverview] = ordersComplemented.usedTasks toKeyedMap { _.id }
  private val jobPathToOverview: Map[JobPath, JobOverview] = ordersComplemented.usedJobs toKeyedMap { _.path }

  protected def title = "Orders"

  override protected def css = s"""
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

  def scalatag = {
    val orderSelection = new OrderSelectionHtml(query)
    htmlPage(
      raw(s"<script type='text/javascript'>${orderSelection.javascript}</script>"),
      orderSelection.html,
      headline,
      ordersStatistics,
      query.jobChainPathQuery.reduce[JobChainPath] match {
        case jobChainPath: JobChainPath ⇒ div(jobChainOrdersHtml(jobChainPath, ordersComplemented.orders))
        case folderPath: FolderPath ⇒ div(folderTreeHtml(FolderTree.fromHasPaths(folderPath, ordersComplemented.orders)))
        case _ ⇒ div(folderTreeHtml(FolderTree.fromHasPaths(FolderPath.Root, ordersComplemented.orders)))
      })
  }

  private def ordersStatistics = {
    val statistics = new OrderOverview.Statistics(ordersComplemented.orders)
    import statistics.{blacklistedCount, count, inProcessCount, suspendedCount}
    p(s"$count orders: $inProcessCount in process using ${jobPathToOverview.size} jobs, $suspendedCount suspended, $blacklistedCount blacklisted")
  }

  def folderTreeHtml(tree: FolderTree[OrderOverview]): immutable.Seq[Frag] =
    Vector(h2("Folder ", folderPathToOrdersA(tree.path)(tree.path.string))) ++
    folderOrdersHtml(tree.leafs map { _.value }) ++
    (for (folder ← tree.subfolders; o ← folderTreeHtml(folder)) yield o)

  private def folderOrdersHtml(orders: immutable.Seq[OrderOverview]): immutable.Iterable[Frag] =
    for ((jobChainPath, jobChainOrders) ← orders groupBy { _.orderKey.jobChainPath };
         o ← jobChainOrdersHtml(jobChainPath, jobChainOrders))
      yield o

  private def jobChainOrdersHtml(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]) =
    Vector(h3(
      s"JobChain ",
      jobChainPathToOrdersA(jobChainPath)(jobChainPath.string),
      span(paddingLeft := 10.px)(" "),
      jobChainPathToA(jobChainPath)("(definition)"))) ++
    nodeOrdersHtml(orders)

  private def nodeOrdersHtml(orders: immutable.Seq[OrderOverview]): immutable.Iterable[Frag] =
    for ((node, orders) ← orders retainOrderGroupBy { _.orderState }) yield
      div(cls := "nodeOrders")(
        div(cls := "nodeHeadline")(s"Node ${node.string}"),
        orderTable(orders))

  private def orderTable(orders: immutable.Seq[OrderOverview]): Frag =
    table(cls := "table table-condensed table-hover")(
      thead(
        colgroup(
          col,
          col,
          col,
          col,
          col(cls := "small")),
        tr(
          th("OrderId"),
          th(div(cls := "visible-lg-block")("SourceType")),
          th("Task / Date"),
          th("Flags"),
          th(small("FileBasedState")))),
      tbody(
        (orders.par map orderToTr).seq))

  private def orderToTr(order: OrderOverview) = {
    val taskOption = for (taskId ← order.taskId) yield {
      val job: Option[JobOverview] = taskIdToOverview.get(taskId) flatMap { task ⇒ jobPathToOverview.get(task.job) }
      val jobPath = job map { _.path.toString } getOrElse "?"
      List(
        b(
          span(cls := "visible-lg-inline")(
            "Task ",
            s"$jobPath:",
            taskToA(taskId)(taskId.string))))
    }
    def nextStepEntry = order.nextStepAt map instantWithDurationToHtml
    tr(cls := orderToTrClass(order))(
      td(order.orderKey.id.string),
      td(div(cls := "visible-lg-block")(order.sourceType.toString)),
      td(taskOption orElse nextStepEntry getOrElse List(EmptyFrag): _*),
      td((order.isSuspended option "suspended") ++ (order.isBlacklisted option "blacklisted") mkString " "),
      td(if (order.sourceType == OrderSourceType.fileBased) fileBasedStateToHtml(order.fileBasedState) else EmptyFrag))
  }

  private def folderPathToOrdersA(path: FolderPath) = queryToA(query.copy(jobChainPathQuery = PathQuery(path)))

  private def jobChainPathToOrdersA(path: JobChainPath) = queryToA(query.copy(jobChainPathQuery = PathQuery(path)))

  private def queryToA(query: OrderQuery) = a(href := uris.order(query, returnType = None))

  private def jobChainPathToA(path: JobChainPath) = a(href := uris.jobChain.details(path))

  private def taskToA(taskId: TaskId) = a(href := uris.task.overview(taskId))
}

object OrdersHtmlPage {

  def toHtml(query: OrderQuery)(ordersComplemented: OrdersComplemented)(implicit context: WebServiceContext, client: DirectSchedulerClient, ec: ExecutionContext): Future[HtmlPage] =
    for (schedulerOverview ← client.overview) yield new OrdersHtmlPage(query, ordersComplemented, context, schedulerOverview)

  private def orderToTrClass(order: OrderOverview) =
    if (order.isSuspended || order.isBlacklisted) "warning"
    else if (order.taskId.isDefined) "info"
    else if (!order.fileBasedState.isOkay) "danger"
    else ""
}
