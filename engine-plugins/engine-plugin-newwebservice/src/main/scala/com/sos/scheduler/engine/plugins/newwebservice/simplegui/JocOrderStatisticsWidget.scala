package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.sprayutils.html.HtmlPage.seqFrag
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.OrderQuery
import scalatags.Text.all._

/**
  * @author Joacim Zschimmer
  */
final class JocOrderStatisticsWidget(uris: SchedulerUris, orderQuery: OrderQuery, markActive: Boolean = false) {

  import orderQuery.nodeQuery.jobChainQuery.pathQuery

  private val fieldGroups: List[(String, List[(String, String, OrderQuery)])] = {
    import OrderProcessingState._
    import OrderSourceType._
    val q = OrderQuery(pathQuery, notInTaskLimitPerNode = orderQuery.notInTaskLimitPerNode)
    List(  // Best layout in small window if groups have same size
      "total" → List(
        ("total", "Total", q),
        ("fileOrder", "FileOrder", q.copy(
          isOrderSourceType = Some(Set(FileOrder)))),
        ("permanent", "Permanent", q.copy(
          isOrderSourceType = Some(Set(Permanent))))),
      "not-started" → List(
        ("notPlanned", "NotPlanned", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(NotPlanned.getClass)))),
        ("planned", "Planned", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[Planned])))),
        ("due", "Due", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[Due]))))),
      "started" → List(
        ("started", "Started", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[WaitingInTask], classOf[InTaskProcess], classOf[OccupiedByClusterMember], WaitingForResource.getClass, classOf[Setback])))),
        ("inTask", "InTask", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[WaitingInTask], classOf[InTaskProcess])))),
        ("inTaskProcess", "InProcess", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[InTaskProcess])))),
        ("waitingForResource", "Waiting", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(WaitingForResource.getClass)))),
        ("setback", "Setback", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[Setback]))))),
      "stalled" → List(
        ("suspended", "Suspended", q.copy(
          isSuspended = Some(true))),
        ("blacklisted", "Blacklisted", q.copy(
          isSuspended = Some(false),
          isBlacklisted = Some(true)))))
  }

  def html: Frag =
    seqFrag(
      onlyHtml,
      raw("<script type='text/javascript'>" + javascript + "</script>"))

  private def onlyHtml: Frag =
    div(id := "OrderStatistics", cls := "ContentBox ")(
      header,
      fields,
      div(clear.left))

  private def header: Frag = {
    val path = pathQuery.typedPath[JobChainPath]
    seqFrag(
      div(cls := "BoxHeader")(
        seqFrag(
          span(cls := "OrderStatistics-Header")(
            "JocOrderStatistics"),
          timestampHtml),
        path != FolderPath.Root option
          span(whiteSpace.nowrap, overflow.hidden, paddingLeft := 1.em)(
            path.string)))
  }

  private def timestampHtml: Frag =
    seqFrag(
      ", ",
      span(cls := "OrderStatistics-pause", title := "Pause", onclick := "jocOrderStatisticsWidget.togglePause()")(
        span(id := "OrderStatistics-pause")(
          span(id := "OrderStatistics-live")(
            "live"),
          span(position.relative, left := (-2.5).ex)(
            span(id := "OrderStatistics-refresh", cls := "glyphicon glyphicon-refresh"))),
        " ",
        span(id := "order-timestamp-value")))

  private def fields: Frag =
    for ((groupCssClass, nameGroup) ← fieldGroups) yield
      div(cls := s"OrderStatistics-fieldGroup OrderStatistics-fieldGroup-$groupCssClass")(
        for ((name, displayName, rawQuery) ← nameGroup;
             query = rawQuery.copy(nodeQuery = rawQuery.nodeQuery.copy(jobChainQuery = rawQuery.jobChainQuery.copy(isDistributed = Some(false)))))
        yield
          div(id := s"order-$name-field", cls := "OrderStatistics-field")(
            a(cls := "inherit-markup", href := uris.order(query, returnType = None))(
              valueField(name, displayName, query))))

  private def valueField(name: String, displayName: String, query: OrderQuery) = {
    val frag =
      div(
        table(cls := "OrderStatistics-value")(
          tbody(
            tr(
              td(displayName),
              td(span(id := s"order-$name-value"))))),
        div(id := s"order-$name-bar", cls := "OrderStatistics-bar", width := 0))
    if (markActive && query == orderQuery)
      span(cls := "OrderStatistics-field-Active")(frag)
    else
      frag
  }

  private def javascript =
    s"jQuery(function() { jocOrderStatisticsWidget.start('${pathQuery.toUriPath}') });"
}
