package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.seqFrag
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.JocOrderStatisticsWidget._
import scalatags.Text.all._

/**
  * @author Joacim Zschimmer
  */
final class JocOrderStatisticsWidget(uris: SchedulerUris, orderQuery: OrderQuery, caption: String = "", markActive: Boolean = false) {

  import orderQuery.nodeQuery.jobChainQuery.pathQuery

  private val fieldGroups: List[List[(String, String, OrderQuery)]] = {
    import OrderProcessingState._
    import OrderSourceType._
    val q = OrderQuery(pathQuery, notInTaskLimitPerNode = orderQuery.notInTaskLimitPerNode)
    List(  // Best layout in small window if groups have same size
      List(
        (TimestampName, TimestampName, orderQuery)),
      List(
        ("total", "total", q),
        ("fileOrder", "fileOrder", q.copy(
          isOrderSourceType = Some(Set(FileOrder)))),
        ("permanent", "permanent", q.copy(
          isOrderSourceType = Some(Set(Permanent))))),
      List(
        ("notPlanned", "notPlanned", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(NotPlanned.getClass)))),
        ("planned", "planned", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[Planned])))),
        ("due", "due", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[Due]))))),
      List(
        ("started", "started", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[WaitingInTask], classOf[InTaskProcess], classOf[OccupiedByClusterMember], WaitingForResource.getClass, classOf[Setback])))),
        ("inTask", "inTask", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[WaitingInTask], classOf[InTaskProcess])))),
        ("inTaskProcess", "inProcess", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[InTaskProcess]))))),
      List(
        ("waitingForResource", "waiting", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(WaitingForResource.getClass)))),
        ("setback", "setback", q.copy(
          isSuspended = Some(false),
          isOrderProcessingState = Some(Set(classOf[Setback])))),
        ("suspended", "suspended", q.copy(
          isSuspended = Some(true))),
        ("blacklisted", "blacklisted", q.copy(
          isSuspended = Some(false),
          isBlacklisted = Some(true)))))
  }

  def html: Frag =
    seqFrag(
      onlyHtml,
      raw("<script type='text/javascript'>" + javascript + "</script>"))

  private def onlyHtml: Frag =
    div(id := "OrderStatistics")(
      header,
      fields,
      div(clear.left))

  private def header: Frag = {
    val path = pathQuery.typedPath[JobChainPath]
    (caption.nonEmpty || path != FolderPath.Root) option
      div(cls := "OrderStatistics-Header")(
        caption != "" option
          a(cls := "inherit-markup", href := uris.order(orderQuery, returnType = None))(
            caption),
        path != FolderPath.Root option seqFrag(" ", path.companion.name, " ", path.string))
  }

  private def fields: Frag =
    for (nameGroup ← fieldGroups) yield
      div(cls := "OrderStatistics-fieldGroup ContentBox")(
        for ((name, displayName, rawQuery) ← nameGroup;
             query = rawQuery.copy(nodeQuery = rawQuery.nodeQuery.copy(jobChainQuery = rawQuery.jobChainQuery.copy(isDistributed = Some(false)))))
        yield
          div(id := s"order-$name-field", cls := "OrderStatistics-field")(
            name == TimestampName option
              span(id := "OrderStatistics-pause", cls := "glyphicon glyphicon-pause", title := "Pause", onclick := "jocOrderStatisticsWidget.togglePause()"),
            a(cls := "inherit-markup", href := uris.order(query, returnType = None))(
              if (name == TimestampName)
                timestampField
              else
                valueField(name, displayName, query))))

  private def timestampField: Frag =
    seqFrag(
      span(id := "order-timestamp-value"),
      span(id := "OrderStatistics-refresh", cls := "glyphicon glyphicon-refresh"))

  private def valueField(name: String, displayName: String, query: OrderQuery) = {
    val frag =
      div(
        span(cls := "OrderStatistics-value")(
          displayName,
          "\u2009",
          span(id := s"order-$name-value")),
        div(id := s"order-$name-bar", cls := "OrderStatistics-bar", width := 0))
    if (markActive && query == orderQuery)
      span(cls := "OrderStatistics-field-Active")(frag)
    else
      frag
  }

  private def javascript =
    s"jQuery(function() { jocOrderStatisticsWidget.start('${pathQuery.toUriPath}') });"
}

object JocOrderStatisticsWidget {
  private val TimestampName = "timestamp"
}
