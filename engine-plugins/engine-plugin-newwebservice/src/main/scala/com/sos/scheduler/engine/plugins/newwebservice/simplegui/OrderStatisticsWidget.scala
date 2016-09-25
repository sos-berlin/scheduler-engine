package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.seqFrag
import scalatags.Text.all._

/**
  * @author Joacim Zschimmer
  */
final class OrderStatisticsWidget(uris: SchedulerUris, orderQuery: OrderQuery, title: String = "") {

  import orderQuery.jobChainQuery.pathQuery

  private val TimestampName = "timestamp"
  private val FieldGroups = {
    import OrderProcessingState._
    import OrderSourceType._
    val q = OrderQuery(jobChainQuery = JobChainQuery(pathQuery = orderQuery.jobChainQuery.pathQuery))
    List(  // Best layout in small window if groups have same size
      List(
        TimestampName → orderQuery),
      List(
        "total" → q,
        "permanent" → q.copy(isOrderSourceType = Some(Set(Permanent))),
        "fileOrder" → q.copy(isOrderSourceType = Some(Set(FileOrder)))),
      List(
        "notPlanned" → q.copy(isOrderProcessingState = Some(Set(NotPlanned.getClass))),
        "planned" → q.copy(isOrderProcessingState = Some(Set(classOf[Planned]))),
        "pending" → q.copy(isOrderProcessingState = Some(Set(classOf[Pending])))),
      List(
        "running" → q.copy(isOrderProcessingState = Some(Set(classOf[WaitingInTask], classOf[InTaskProcess], classOf[OccupiedByClusterMember], WaitingForOther.getClass, classOf[Setback]))),
        "inTask" → q.copy(isOrderProcessingState = Some(Set(classOf[WaitingInTask], classOf[InTaskProcess], classOf[OccupiedByClusterMember]))),
        "inProcess" → q.copy(isOrderProcessingState = Some(Set(classOf[InTaskProcess])))),
      List(
        "setback" → q.copy(isOrderProcessingState = Some(Set(classOf[Setback]))),
        "suspended" → q.copy(isSuspended = Some(true)),
        "blacklisted" → q.copy(isBlacklisted = Some(true))))
  }

  def html: Frag =
    seqFrag(
      onlyHtml,
      raw("<script type='text/javascript'>" + javascript + "</script>"))

  private def onlyHtml: Frag =
    div(id := "OrderStatistics", cls := "ContentBox")(
      header,
      fields,
      div(clear.left))

  private def header: Frag = {
    val path = pathQuery.typedPath[JobChainPath]
    (title.nonEmpty || path != FolderPath.Root) option
      div(cls := "OrderStatistics-Header")(
        title != "" option
          a(cls := "inherit-markup", href := uris.order(orderQuery, returnType = None))(
            title),
        path != FolderPath.Root option seqFrag(" ", path.companion.name, " ", path.string))
  }

  private def fields: Frag =
    for (nameGroup ← FieldGroups) yield
      div(cls := "OrderStatistics-fieldGroup")(
        for ((name, query) ← nameGroup) yield
          div(id := s"order-$name-field", cls := "OrderStatistics-field")(
            a(cls := "inherit-markup", href := uris.order(query, returnType = None))(
              if (name == TimestampName)
                timestampField
              else
                seqFrag(
                  name,
                  "\u2009",
                  span(id := s"order-$name-value")))))

  private def timestampField: Frag =
    seqFrag(
      span(id := "order-timestamp-value"),
      span(id := "OrderStatistics-refresh", cls := "glyphicon glyphicon-refresh"))

  private def javascript =
    s"jQuery(function() { startOrderStatisticsChangedListener('${pathQuery.toUriPath}') });"
}
