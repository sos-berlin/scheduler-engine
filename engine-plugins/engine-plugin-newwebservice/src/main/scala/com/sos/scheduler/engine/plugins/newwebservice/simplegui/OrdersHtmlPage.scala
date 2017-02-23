package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.scalautil.Collections.implicits.RichTraversable
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.jobscheduler.data.event.Snapshot
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.compounds.OrdersComplemented
import com.sos.scheduler.engine.data.folder.FolderTree
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobNodeOverview, NodeKey, NodeObstacle}
import com.sos.scheduler.engine.data.order.OrderProcessingState._
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderOverview}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.{EmptyFrag, joinHtml, seqFrag}
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder.toVersionedUriPath
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.OrdersHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage._
import java.time.Instant.EPOCH
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}
import scalatags.Text.all._
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
  //private val taskIdToOverview: Map[TaskId, TaskOverview] = ordersComplemented.usedTasks toKeyedMap { _.id }
  private val jobPathToOverview: Map[JobPath, JobOverview] = ordersComplemented.usedJobs toKeyedMap { _.path }
  private val jobPathToObstacleHtml: Map[JobPath, Option[Frag]] = ordersComplemented.usedJobs.toKeyedMap { _.path }
    .mapValues { jobOverview ⇒
      jobOverview.obstacles.nonEmpty option
        seqFrag(jobOverview.path.toString, ": ", dotJoin(jobOverview.obstacles map { o ⇒ stringFrag(o.toString) }))
    }
    .withDefault { jobPath ⇒ Some(span(cls := "text-danger")(seqFrag("Missing ", jobPath.toString))) }
  private val nodeKeyToOverview: Map[NodeKey, JobNodeOverview] = ordersComplemented.usedNodes toKeyedMap { _.nodeKey }
  private val nodeKeyToObstacleHtml: Map[NodeKey, Option[Frag]] = nodeKeyToOverview.mapValues {
      case node if node.obstacles.nonEmpty ⇒
        val obstacles = (node.obstacles map { o ⇒ stringFrag(o.toString) }) ++
          (node.obstacles contains NodeObstacle.WaitingForJob option
            jobPathToObstacleHtml(node.jobPath)).flatten
        Some(seqFrag(
          "Node: ",
          dotJoin(obstacles)))
      case _ ⇒
        None
    }
    .withDefaultValue(Some(span(cls := "text-danger")(stringFrag("Missing Node"))))

  override protected def pageTitle = "Orders"
  override protected def cssPaths = super.cssPaths ++ CssPaths
  override protected def scriptPaths = super.scriptPaths ++ ScriptPaths

  def wholePage = {
    htmlPage(
      div(float.left)(
        orderStatisticsWidget.html),
      div(float.right)(
        orderSelectionStatistics),
      div(float.right)(
        new OrderSelectionWidget(queryToUri, query).html),
      div(clear.both)(
        query.jobChainQuery.pathQuery match {
          case single: PathQuery.SinglePath ⇒ div(jobChainOrdersToHtml(single.as[JobChainPath], ordersComplemented.orders))
          case PathQuery.Folder(folderPath, ignoringIsRecursive) ⇒ div(wholeFolderTreeToHtml(FolderTree.fromHasPaths(folderPath, ordersComplemented.orders)))
          case _ ⇒ div(wholeFolderTreeToHtml(FolderTree.fromHasPaths(FolderPath.Root, ordersComplemented.orders)))
        }))
  }

  private def orderStatisticsWidget = new JocOrderStatisticsWidget(uris, query, markActive = true)

  private def orderSelectionStatistics = {
    val statistics = new OrderOverview.Statistics(ordersComplemented.orders)
    import statistics.{blacklistedCount, count, inProcessCount, suspendedCount}
    div(cls := "ContentBox OrderSelectionStatistics")(
      div(paddingTop := 4.px),
      table(cls := "MiniTable")(
        thead(tr(th, th(cls := "BoxHeader")("Showed"))),
        tbody(
          tr(td(textAlign.right, width := 5.ex)(s"$count")     , td("orders")),
          tr(td(textAlign.right)(s"$inProcessCount")           , td("in process")),
          tr(td(textAlign.right)(s"${jobPathToOverview.size}") , td("job")),
          tr(td(textAlign.right)(s"$suspendedCount")           , td("suspended")),
          tr(td(textAlign.right)(s"$blacklistedCount")         , td("blacklisted")))))
  }

  private def wholeFolderTreeToHtml(tree: FolderTree[OrderOverview]): Frag =
    if (tree.isEmpty)
      div(cls := "Padded", paddingTop := 4.em)(
        "No order meets selection criteria.")
    else
      folderTreeToHtml(tree)

  private def folderTreeToHtml(tree: FolderTree[OrderOverview]): Frag =
    seqFrag(
      folderToHtml(tree.path, tree.leafs),
      for (folder ← tree.subfolders) yield folderTreeToHtml(folder))

  private def folderToHtml(folderPath: FolderPath, orders: immutable.Seq[OrderOverview]) =
    if (orders.isEmpty)
      EmptyFrag
    else
      seqFrag(
        h2(cls := "Folder Padded")(
          folderPathToOrdersA(folderPath)("Folder ", folderPath.string)),
        folderOrdersToHtml(orders))

  private def folderOrdersToHtml(orders: immutable.Seq[OrderOverview]): Frag =
    for ((jobChainPath, jobChainOrders) ← orders retainOrderGroupBy { _.orderKey.jobChainPath })
      yield jobChainOrdersToHtml(jobChainPath, jobChainOrders)

  private def jobChainOrdersToHtml(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]) =
    div(cls := "ContentBox JobChain", clear.both)(
      div(cls := "Padded")(
        div(float.right)(
          jobChainPathToA(jobChainPath)("→definition")),
        h3(cls := "JobChain")(
          jobChainPathToOrdersA(jobChainPath)("JobChain ", jobChainPath.string),
          span(paddingLeft := 10.px)(" "))),
      nodeOrdersToHtml(jobChainPath, orders))

  private def nodeOrdersToHtml(jobChainPath: JobChainPath, orders: immutable.Seq[OrderOverview]): Frag = {
    for ((nodeId, orders) ← orders retainOrderGroupBy { _.nodeId }) yield {
      val jobPath = nodeKeyToOverview(NodeKey(jobChainPath, nodeId)).jobPath
      div(cls := "NodeOrders")(
        div(cls := "Padded NodeHeadline")("Node ", nodeId.string, " \u00a0 Job ", jobPath.string),
        ordersToTable(orders))
    }
  }

  private def ordersToTable(orders: immutable.Seq[OrderOverview]): Frag =
    table(cls := "table table-condensed table-hover")(
      thead(
        tr(
          th("OrderId"),
          th("Started"),
          th("OrderProcessingState"),
          th("Obstacles"))),
      tbody(
        (orders.par map orderToTr).seq))

  private def orderToTr(order: OrderOverview) = {
    val processingStateHtml: Frag = order.orderProcessingState match {
      case Planned(at) ⇒ instantWithDurationToHtml(at)
      case Due(at) ⇒ seqFrag("Due", if (at == EPOCH) EmptyFrag else seqFrag(" since ", instantWithDurationToHtml(at)))
      case Setback(at) ⇒ seqFrag("Setback, next try ", instantWithDurationToHtml(at))
      case inTask: InTask ⇒
        val taskId = inTask.taskId
        val jobPath = nodeKeyToOverview.get(order.nodeKey) map { _.jobPath.string } getOrElse "(unknown job)"
        val taskHtml = b(taskToA(taskId)("Task ", taskId.string))
        inTask match {
          case _: WaitingInTask ⇒ seqFrag(taskHtml, " WaitingInTask")
          case o: InTaskProcess ⇒ seqFrag(taskHtml, " InTaskProcess since ", shortInstantWithDurationToHtml(o.since))
       }
      case o ⇒ stringFrag(o.toString)
    }
    val occupyingMemberHtml = order.occupyingClusterMemberId map { o ⇒ seqFrag(", occupied by ", o.toString) }
    val nodeObstaclesHtml: Option[Frag] = order.orderProcessingState match {
      case _: InTask ⇒ None
      case _ ⇒ nodeKeyToObstacleHtml(order.nodeKey)
    }
    val obstaclesHtml: Frag = {
      val orderObstaclesHtml = order.obstacles.toVector map { o ⇒ stringFrag(o.toString) }
      val htmls = orderObstaclesHtml ++ nodeObstaclesHtml
      htmls.nonEmpty option {
        val joined = dotJoin(htmls)
        if (order.orderProcessingState.isDueOrStarted)
          span(cls := "text-danger")(joined)
        else
          joined
      }
    }
    val rowCssClass = orderToTrClass(order) getOrElse (if (order.orderProcessingState.isWaiting && nodeObstaclesHtml.nonEmpty) "warning" else "")
    tr(cls := s"$rowCssClass clickable", data("href") := uris.order[OrderDetailed](order.orderKey))(
      td(order.orderKey.id.string),
      td(order.startedAt map instantWithDurationToHtml),
      td(processingStateHtml, occupyingMemberHtml),
      td(obstaclesHtml))
  }

  private def folderPathToOrdersA(path: FolderPath) =
    queryToA(query.copy(nodeQuery = JobChainNodeQuery(jobChainQuery = query.jobChainQuery.copy(pathQuery = PathQuery(path)))))

  private def jobChainPathToOrdersA(path: JobChainPath) =
    queryToA(query.copy(nodeQuery = JobChainNodeQuery(jobChainQuery = query.jobChainQuery.copy(pathQuery = PathQuery(path)))))

  private def queryToA(query: OrderQuery) = hiddenA(queryToUri(query))

  private def queryToUri(query: OrderQuery) = uris.order(query, returnType = None)

  private def jobChainPathToA(path: JobChainPath) = hiddenA(uris.jobChain.detailed(path))

  private def taskToA(taskId: TaskId) = hiddenA(uris.task.overview(taskId))

  private def hiddenA(url: String) = a(cls := "inherit-markup", href := url)
}

object OrdersHtmlPage {

  private val Dot = '\u00b7'
  private val CssPaths = Vector(
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/common/OrderStatisticsWidget.css")),
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/order/OrderSelectionWidget.css")),
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/order/order.css")))
  private val ScriptPaths = Vector(
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/common/OrderStatisticsWidget.js")),
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/order/OrderSelectionWidget.js")))

  def toHtmlPage(
    snapshot: Snapshot[OrdersComplemented[OrderOverview]],
    pageUri: Uri,
    query: OrderQuery,
    client: SchedulerOverviewClient,
    webServiceContext: WebServiceContext)
    (implicit ec: ExecutionContext): Future[OrdersHtmlPage]
  =
    for (schedulerOverviewResponse ← client.overview) yield
      new OrdersHtmlPage(snapshot, pageUri, query, schedulerOverviewResponse.value, webServiceContext.uris)

  private def orderToTrClass(order: OrderOverview): Option[String] =
    order.orderProcessingState match {
      case NotPlanned | _: Planned ⇒ None
      case _: Due ⇒ Some("Order-Due-Suspended")
      case _: Due | _: Started | _: WaitingInTask if order.isSuspended ⇒ Some("Order-Suspended")
      case o ⇒ Some(s"Order-${o.getClass.getSimpleName stripSuffix "$"}")
    }

  private def dotJoin(frags: Iterable[Frag]) = joinHtml(s" $Dot ")(frags)
}
