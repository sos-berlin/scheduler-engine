package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersComplemented
import com.sos.scheduler.engine.data.folder.{FolderPath, FolderTree}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobNodeOverview, NodeKey}
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

  private val nodeKeyToOverview: Map[NodeKey, JobNodeOverview] = ordersComplemented.usedNodes toKeyedMap { _.nodeKey }
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
div.OrderStatistics {
  background-color: white;
  margin-right: 30px;
  padding: 0 4px 4px 4px;
  border: 1px solid #c0c0c0;
}
div.OrderSelection {
  margin-right: 20px;
  margin-bottom: 10px;
  background-color: #f9d186;
  line-height: 1em;
  padding: 0 4px 4px 4px;
  border: 1px solid #c0c0c0;
}
div.NodeHeadline {
  padding: 5px 0 0 5px;
  font-weight: bold;
}
div.NodeOrders {
  margin-bottom: 5px;
  border-radius: 2px;
}
""" + super.css

  def scalatag = {
    val orderSelection = new OrderSelectionHtml(query)
    htmlPage(
      raw(s"<script type='text/javascript'>${orderSelection.javascript}</script>"),
      div(float.right)(
        ordersStatistics),
      div(float.right)(
        orderSelection.html),
      headline,
      query.jobChainPathQuery match {
        case single: PathQuery.SinglePath ⇒ div(jobChainOrdersHtml(single.as[JobChainPath], ordersComplemented.orders))
        case PathQuery.Folder(folderPath) ⇒ div(folderTreeHtml(FolderTree.fromHasPaths(folderPath, ordersComplemented.orders)))
        case _ ⇒ div(folderTreeHtml(FolderTree.fromHasPaths(FolderPath.Root, ordersComplemented.orders)))
      })
  }

  private def ordersStatistics = {
    val statistics = new OrderOverview.Statistics(ordersComplemented.orders)
    import statistics.{blacklistedCount, count, inProcessCount, suspendedCount}
    div(cls := "OrderStatistics")(
      div(paddingTop := 4.px),
      table(cls := "MiniTable")(
        tbody(
          tr(td(textAlign.right)(s"$count")                    , td(s" orders")),
          tr(td(textAlign.right)(s"$inProcessCount")           , td(s"in process")),
          tr(td(textAlign.right)(s"${ jobPathToOverview.size}"), td(s"jobs")),
          tr(td(textAlign.right)(s"$suspendedCount")           , td(s"suspended")),
          tr(td(textAlign.right)(s"$blacklistedCount")         , td(s"blacklisted")))))
  }

  private def folderTreeHtml(tree: FolderTree[OrderOverview]): immutable.Seq[Frag] =
    Vector(folderHtml(tree.path, tree.leafs)) ++
    (for (folder ← tree.subfolders; o ← folderTreeHtml(folder)) yield o)

  private def folderHtml(folderPath: FolderPath, orders: immutable.Seq[OrderOverview]) =
    div(cls := "ContentBox")(
      Vector(h2("Folder ", folderPathToOrdersA(folderPath)(folderPath.string))) ++
      folderOrdersHtml(orders))

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
      div(cls := "NodeOrders")(
        div(cls := "NodeHeadline")(s"Node ${node.string}"),
        orderTable(orders))

  private def orderTable(orders: immutable.Seq[OrderOverview]): Frag =
    table(cls := "table table-condensed table-hover")(
      colgroup(
        col,
        col,
        col,
        col,
        col(cls := "small")),
      thead(
        tr(
          th("OrderId"),
          th(div(cls := "visible-lg-block")("SourceType")),
          th("OrderProcessingState"),
          th("Obstacles"),
          th(small("FileBasedState")))),
      tbody(
        (orders.par map orderToTr).seq))

  private def orderToTr(order: OrderOverview) = {
    val jobPathOption = nodeKeyToOverview.get(order.nodeKey) map { _.jobPath }
    val processingStateHtml: immutable.Seq[Frag] =
      order.processingState match {
        case OrderProcessingState.Planned(at) ⇒ instantWithDurationToHtml(at)
        case OrderProcessingState.Pending(at) ⇒ (at != EPOCH list instantWithDurationToHtml(at)) ++ List(List(stringFrag("pending"))) reduce { _ ++ List(stringFrag(" ")) ++ _ }
        case OrderProcessingState.Setback(at) ⇒ "Set back until " :: instantWithDurationToHtml(at)
        case inTask: OrderProcessingState.InTask ⇒
          val taskId = inTask.taskId
          val jobPath = jobPathOption getOrElse "(unknown job)"
          val taskHtml = List(
            b(
              span(cls := "visible-lg-inline")(
                "Task ",
                s"$jobPath:",
                taskToA(taskId)(taskId.string))))
          inTask match {
            case _: OrderProcessingState.WaitingInTask ⇒ taskHtml ++ List(stringFrag(" waiting for process"))
            case _: OrderProcessingState.InTaskProcess ⇒ taskHtml
         }
        case o ⇒ List(stringFrag(o.toString))
      }
    val jobObstaclesHtml: List[Frag] =
      order.processingState match {
        case _: OrderProcessingState.InTask ⇒ Nil
        case _ ⇒ jobPathOption.toList flatMap jobPathToObstacleHtml
      }
    val obstaclesHtml: List[Frag] = {
      val inner = List(List(stringFrag(order.obstacles mkString " ")), jobObstaclesHtml) reduce { _ ++ List(stringFrag(" ")) ++ _ }
      if (inner.isEmpty) Nil else span(cls := "text-danger")(inner) :: Nil
    }
    val rowCssClass = {
      def isWarning = order.processingState.isInstanceOf[OrderProcessingState.Waiting] && jobObstaclesHtml.nonEmpty
      orderToTrClass(order) getOrElse (if (isWarning) "warning" else "")
    }
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
