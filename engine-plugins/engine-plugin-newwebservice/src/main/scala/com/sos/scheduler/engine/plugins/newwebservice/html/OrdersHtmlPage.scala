package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersComplemented
import com.sos.scheduler.engine.data.folder.{FolderPath, FolderTree}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import java.time.Instant.EPOCH
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

  //private val taskIdToOverview: Map[TaskId, TaskOverview] = ordersComplemented.usedTasks toKeyedMap { _.id }
  private val jobPathToOverview: Map[JobPath, JobOverview] = ordersComplemented.usedJobs toKeyedMap { _.path }
  private val jobPathToObstacleHtml: Map[JobPath, List[Frag]] = ordersComplemented.usedJobs.toKeyedMap { _.path }
    .mapValues { jobOverview ⇒
      if (jobOverview.obstacles.nonEmpty)
        List(stringFrag(s"${jobOverview.path}: " + jobOverview.obstacles.mkString(" ")))
      else
        Nil
    }
    .withDefault { jobPath ⇒ List(span(cls := "text-danger")(stringFrag(s"Missing $jobPath"))) }

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
      query.jobChainPathQuery match {
        case single: PathQuery.SinglePath ⇒ div(jobChainOrdersHtml(single.as[JobChainPath], ordersComplemented.orders))
        case PathQuery.Folder(folderPath) ⇒ div(folderTreeHtml(FolderTree.fromHasPaths(folderPath, ordersComplemented.orders)))
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
    folderOrdersHtml(tree.leafs) ++
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
          th("OrderProcessingState"),
          th("Obstacles"),
          th(small("FileBasedState")))),
      tbody(
        (orders.par map orderToTr).seq))

  private def orderToTr(order: OrderOverview) = {
    val processingStateHtml: immutable.Seq[Frag] =
      order.processingState match {
        case OrderProcessingState.Planned(at) ⇒ instantWithDurationToHtml(at)
        case OrderProcessingState.Pending(at) ⇒ (at != EPOCH list instantWithDurationToHtml(at)) ++ List(List(stringFrag("pending"))) reduce { _ ++ List(stringFrag(" ")) ++ _ }
        case OrderProcessingState.Setback(at) ⇒ "Set back until " :: instantWithDurationToHtml(at)
        case inTask: OrderProcessingState.InTask ⇒
          val taskId = inTask.taskId
          val jobPath = order.jobPath getOrElse "(unknown job)"
          val taskHtml = List(
            b(
              span(cls := "visible-lg-inline")(
                "Task ",
                s"$jobPath:",
                taskToA(taskId)(taskId.string))))
          inTask match {
            case OrderProcessingState.WaitingInTask(_) ⇒ taskHtml ++ List(stringFrag(" waiting for process"))
            case OrderProcessingState.InTaskProcess(_) ⇒ taskHtml
         }
        case o ⇒ List(stringFrag(o.toString))
      }
    val jobObstaclesHtml: List[Frag] =
      order.processingState match {
        case _: OrderProcessingState.InTask ⇒ Nil
        case _ ⇒ span(cls := "text-danger")(order.jobPath.toList flatMap jobPathToObstacleHtml) :: Nil
      }
    val obstaclesHtml: List[Frag] = List(List(stringFrag(order.obstacles mkString " ")), jobObstaclesHtml) reduce { _ ++ List(stringFrag(" ")) ++ _ }
    val rowCssClass = orderToTrClass(order) getOrElse (if (jobObstaclesHtml.nonEmpty) "warning" else "")
    tr(cls := rowCssClass)(
      td(order.orderKey.id.string),
      td(div(cls := "visible-lg-block")(order.sourceType.toString)),
      td(processingStateHtml),
      td(obstaclesHtml),
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

  private def orderToTrClass(order: OrderOverview): Option[String] =
    if (order.obstacles.nonEmpty)
      Some("bg-warning")
    else
      order.processingState match {
        case _: OrderProcessingState.InTaskProcess ⇒ Some("bg-primary")
        case _: OrderProcessingState.Pending ⇒ Some("bg-info")
        case _ if !order.fileBasedState.isOkay ⇒ Some("bg-danger")
        case _ ⇒ None
      }
}
