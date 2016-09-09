package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichTraversable
import com.sos.scheduler.engine.data.compounds.OrdersComplemented
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.folder.{FolderPath, FolderTree}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobNodeOverview, NodeKey}
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderKey, OrderOverview, OrderProcessingState}
import com.sos.scheduler.engine.data.queries.{OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.joinHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.OrdersHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage._
import java.time.Instant.EPOCH
import scala.collection.immutable
import scala.concurrent.ExecutionContext
import scalatags.Text.all._
import scalatags.text.Frag
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class OrdersHtmlPage private(
  protected val snapshot: Snapshot[OrdersComplemented[OrderOverview]],
  protected val pageUri: Uri,
  query: OrderQuery,
  protected val schedulerOverview: SchedulerOverview,
  protected val uris: SchedulerUris)
extends SchedulerHtmlPage {

  private val ordersComplemented: OrdersComplemented[OrderOverview] = snapshot.value
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
  override protected def cssLinks = super.cssLinks ++ List(
    uris / "api/frontend/common/OrderStatisticsWidget.css",
    uris / "api/frontend/common/OrderSelectionWidget.css",
    uris / "api/frontend/order/order.css")
  override protected def scriptLinks = super.scriptLinks ++ List(
    uris / "api/frontend/common/OrderStatisticsWidget.js",
    uris / "api/frontend/order/OrderSelectionWidget.js")

  def wholePage = {
    htmlPage(
      div(cls := "Padded")(
        OrderStatisticsWidget.html),
      div(float.right)(
        orderSelectionStatistics),
      div(float.right)(
        new OrderSelectionWidget(query).html),
      query.jobChainPathQuery match {
        case single: PathQuery.SinglePath ⇒ div(jobChainOrdersToHtml(single.as[JobChainPath], ordersComplemented.orders))
        case PathQuery.Folder(folderPath) ⇒ div(wholeFolderTreeToHtml(FolderTree.fromHasPaths(folderPath, ordersComplemented.orders)))
        case _ ⇒ div(wholeFolderTreeToHtml(FolderTree.fromHasPaths(FolderPath.Root, ordersComplemented.orders)))
      })
  }

  private def orderSelectionStatistics = {
    val statistics = new OrderOverview.Statistics(ordersComplemented.orders)
    import statistics.{blacklistedCount, count, inProcessCount, suspendedCount}
    div(cls := "ContentBox OrderSelectionStatistics")(
      div(paddingTop := 4.px),
      table(cls := "MiniTable")(
        tbody(
          tr(td(textAlign.right)(s"$count")                    , td(s" orders")),
          tr(td(textAlign.right)(s"$inProcessCount")           , td(s"in process")),
          tr(td(textAlign.right)(s"${ jobPathToOverview.size}"), td(s"jobs")),
          tr(td(textAlign.right)(s"$suspendedCount")           , td(s"suspended")),
          tr(td(textAlign.right)(s"$blacklistedCount")         , td(s"blacklisted")))))
  }

  private def wholeFolderTreeToHtml(tree: FolderTree[OrderOverview]): immutable.Seq[Frag] =
    if (tree.isEmpty)
      List(div(cls := "Padded", paddingTop := 4.em)(
        "No order meets selection criteria."))
    else
      folderTreeToHtml(tree)

  private def folderTreeToHtml(tree: FolderTree[OrderOverview]): immutable.Seq[Frag] =
    folderToHtml(tree.path, tree.leafs) ++
    (for (folder ← tree.subfolders; o ← folderTreeToHtml(folder)) yield o)

  private def folderToHtml(folderPath: FolderPath, orders: immutable.Seq[OrderOverview]) =
    if (orders.isEmpty)
      Nil
    else
      Vector(
        h2(cls := "Folder Padded")(
          folderPathToOrdersA(folderPath)(s"Folder ${folderPath.string}"))) ++
      folderOrdersToHtml(orders)

  private def folderOrdersToHtml(orders: immutable.Seq[OrderOverview]): Vector[Frag] =
    for ((jobChainPath, jobChainOrders) ← orders retainOrderGroupBy { _.orderKey.jobChainPath };
         o ← jobChainOrdersToHtml(jobChainPath, jobChainOrders))
      yield o

  private def jobChainOrdersToHtml(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]) =
    List(
      div(cls := "ContentBox JobChain", clear.both)(
        div(cls := "Padded")(
          div(float.right)(
            jobChainPathToA(jobChainPath)("→definition")),
          h3(cls := "JobChain")(
            jobChainPathToOrdersA(jobChainPath)(s"JobChain ${jobChainPath.string}"),
            span(paddingLeft := 10.px)(" "))),
        nodeOrdersToHtml(jobChainPath, orders)))

  private def nodeOrdersToHtml(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]): Vector[Frag] =
    for ((nodeId, orders) ← orders retainOrderGroupBy { _.nodeId }) yield {
      val jobPath = nodeKeyToOverview(NodeKey(jobChainPath, nodeId)).jobPath
      div(cls := "NodeOrders")(
        div(cls := "Padded")(
          div(cls := "NodeHeadline")(s"Node ${nodeId.string} \u00a0 ${jobPath.string}")),
        ordersToTable(orders))
    }

  private def ordersToTable(orders: immutable.Seq[OrderOverview]): Frag =
    table(cls := "table table-condensed table-hover")(
      colgroup(
        col,
        col,
        col,
        col),
      thead(
        tr(
          th("OrderId"),
          th(div(cls := "visible-lg-block")("SourceType")),
          th("Started"),
          th("OrderProcessingState"),
          th("Obstacles"))),
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
    val obstaclesHtml: Frag = {
      val orderObstaclesHtml = order.obstacles.toList map { o ⇒ stringFrag(o.toString) }
      orderObstaclesHtml ++ jobObstaclesHtml match {
        case obstacles if obstacles.nonEmpty ⇒ span(cls := "text-danger")(joinHtml(s" $Dot ")(obstacles))
        case _ ⇒ StringFrag("")
      }
    }
    val rowCssClass = orderToTrClass(order) getOrElse (if (isWaiting && jobObstaclesHtml.nonEmpty) "warning" else "")
    tr(cls := rowCssClass)(
      td(orderKeyToA(order.orderKey)(order.orderKey.id.string)),
      td(div(cls := "visible-lg-block")(order.sourceType.toString)),
      td(order.startedAt map instantWithDurationToHtml),
      td(processingStateHtml, occupyingMemberHtml),
      td(obstaclesHtml))
  }

  private def folderPathToOrdersA(path: FolderPath) = queryToA(query.copy(jobChainPathQuery = PathQuery(path)))

  private def jobChainPathToOrdersA(path: JobChainPath) = queryToA(query.copy(jobChainPathQuery = PathQuery(path)))

  private def queryToA(query: OrderQuery) = hiddenA(uris.order(query, returnType = None))

  private def jobChainPathToA(path: JobChainPath) = hiddenA(uris.jobChain.details(path))

  private def orderKeyToA(orderKey: OrderKey) = hiddenA(uris.order[OrderDetailed](orderKey))

  private def taskToA(taskId: TaskId) = hiddenA(uris.task.overview(taskId))

  private def hiddenA(url: String) = a(cls := "inherit-markup", href := url)
}

object OrdersHtmlPage {

  private val Dot = '\u00b7'

  def toHtmlPage(
    snapshot: Snapshot[OrdersComplemented[OrderOverview]],
    pageUri: Uri,
    query: OrderQuery,
    client: SchedulerOverviewClient,
    webServiceContext: WebServiceContext)
    (implicit ec: ExecutionContext)
  =
    for (schedulerOverviewResponse ← client.overview) yield
      new OrdersHtmlPage(snapshot, pageUri, query, schedulerOverviewResponse.value, webServiceContext.uris)

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
