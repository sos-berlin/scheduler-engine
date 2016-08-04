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

  override protected def title = "Orders"

  override protected def css = s"""
h2.Folder {
  margin-top: 3em;
}
h3.JobChain {
  margin-top: 0;
  padding-top: 10px;
}
.OrderStatistics {
  background-color: white;
  margin-right: 30px;
  padding: 0 4px 4px 4px;
  border: 1px solid #c0c0c0;
}
.OrderSelection {
  margin-right: 20px;
  margin-bottom: 10px;
  background-color: #f9d186;
  line-height: 1em;
  padding: 0 4px 4px 4px;
  border: 1px solid #c0c0c0;
}
.NodeHeadline {
  margin: 2em 0 1em;
  font-weight: bold;
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
    if (tree.isEmpty)
      List(div(cls := "Padded")("Order selection is empty"))
    else
      folderTreeHtml2(tree)

  private def folderTreeHtml2(tree: FolderTree[OrderOverview]): immutable.Seq[Frag] =
    folderHtml(tree.path, tree.leafs) ++
    (for (folder ← tree.subfolders; o ← folderTreeHtml2(folder)) yield o)

  private def folderHtml(folderPath: FolderPath, orders: immutable.Seq[OrderOverview]) =
    if (orders.isEmpty)
      Nil
    else
      Vector(
        h2(cls := "Folder Padded")(
          folderPathToOrdersA(folderPath)(s"Folder ${folderPath.string}"))) ++
      folderOrdersHtml(orders)

  private def folderOrdersHtml(orders: immutable.Seq[OrderOverview]): Vector[Frag] =
    for ((jobChainPath, jobChainOrders) ← orders retainOrderGroupBy { _.orderKey.jobChainPath };
         o ← jobChainOrdersHtml(jobChainPath, jobChainOrders))
      yield o

  private def jobChainOrdersHtml(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]) =
    List(
      div(cls := "ContentBox", clear.both)(
        div(cls := "Padded")(
          div(float.right)(
            jobChainPathToA(jobChainPath)("→definition")),
          h3(cls := "JobChain")(
            jobChainPathToOrdersA(jobChainPath)(s"JobChain ${jobChainPath.string}"),
            span(paddingLeft := 10.px)(" "))),
        nodeOrdersHtml(jobChainPath, orders)))

  private def nodeOrdersHtml(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]): Vector[Frag] =
    for ((nodeId, orders) ← orders retainOrderGroupBy { _.orderState }) yield {
      val jobPath = nodeKeyToOverview(NodeKey(jobChainPath, nodeId)).jobPath
      div(cls := "NodeOrders")(
        div(cls := "Padded")(
          div(cls := "NodeHeadline")(s"Node ${nodeId.string} \u00a0 ${jobPath.string}")),
        orderTable(orders))
    }

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
          val jobPath = jobPathOption map { _.string } getOrElse "(unknown job)"
          val taskHtml = List(
            b(
              span(cls := "visible-lg-inline")(
                taskToA(taskId)(s"Task $jobPath:${taskId.string}"))))
          inTask match {
            case _: OrderProcessingState.WaitingInTask ⇒ taskHtml ++ List(stringFrag(" waiting for process"))
            case o: OrderProcessingState.InTaskProcess ⇒ taskHtml ::: stringFrag(" since ") :: instantWithDurationToHtml(o.since)
         }
        case o ⇒ List(stringFrag(o.toString))
      }
    val occupyingMemberHtml = order.occupyingClusterMemberId map { o ⇒ stringFrag(s", occupied by $o") }
    val jobObstaclesHtml: List[Frag] = order.processingState match {
      case _: OrderProcessingState.InTask ⇒ Nil
      case _ ⇒ jobPathOption.toList flatMap jobPathToObstacleHtml
    }
    val isWaiting = order.processingState.isInstanceOf[OrderProcessingState.Waiting]
    val obstaclesHtml: List[Frag] = {
      val inner = List(List(stringFrag(order.obstacles mkString " ")), jobObstaclesHtml) reduce { _ ++ List(stringFrag(" ")) ++ _ }
      if (isWaiting && inner.nonEmpty)
        span(cls := "text-danger")(inner) :: Nil
      else
        inner
    }
    val rowCssClass = orderToTrClass(order) getOrElse (if (isWaiting && jobObstaclesHtml.nonEmpty) "warning" else "")
    tr(cls := rowCssClass)(
      td(order.orderKey.id.string),
      td(div(cls := "visible-lg-block")(order.sourceType.toString)),
      td(processingStateHtml, occupyingMemberHtml),
      td(obstaclesHtml),
      td(if (order.sourceType == OrderSourceType.fileBased) fileBasedStateToHtml(order.fileBasedState) else EmptyFrag))
  }

  private def folderPathToOrdersA(path: FolderPath) = queryToA(query.copy(jobChainPathQuery = PathQuery(path)))

  private def jobChainPathToOrdersA(path: JobChainPath) = queryToA(query.copy(jobChainPathQuery = PathQuery(path)))

  private def queryToA(query: OrderQuery) = a(cls := "inherit-markup", href := uris.order(query, returnType = None))

  private def jobChainPathToA(path: JobChainPath) = a(cls := "inherit-markup", href := uris.jobChain.details(path))

  private def taskToA(taskId: TaskId) = a(cls := "inherit-markup", href := uris.task.overview(taskId))
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
