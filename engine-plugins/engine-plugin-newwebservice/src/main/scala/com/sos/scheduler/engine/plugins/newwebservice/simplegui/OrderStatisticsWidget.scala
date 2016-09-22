package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.seqFrag
import scalatags.Text.all._

/**
  * @author Joacim Zschimmer
  */
class OrderStatisticsWidget(pathQuery: PathQuery) {

  private val TimestampName = "timestamp"
  private val FieldGroups = List(
    List(
      "total",
      "notPlanned",
      "planned",
      "pending"),
    List(
      "running",
      "inTask",
      "inProcess"),
    List(
      "setback",
      "suspended",
      "blacklisted"),
    List(
      "permanent",
      "fileOrder",
      TimestampName))

  final def html: Frag =
    seqFrag(
      onlyHtml,
      raw("<script type='text/javascript'>" + javascript + "</script>"))

  private def onlyHtml =
    div(id := "OrderStatistics", cls := "ContentBox")(
      div(cls := "OrderStatistics-Header")(
        "Live ",
        !pathQuery.matchesAll option {
          val path = pathQuery.typedPath[JobChainPath]
          seqFrag(path.companion.name, " ", path.string)
        }),
      for (nameGroup ← FieldGroups) yield
        div(cls := "OrderStatistics-fieldGroup")(
          for (name ← nameGroup) yield
            if (name != TimestampName)
              div(id := s"order-$name-field", cls := "OrderStatistics-field")(
                s"$name:\u2009",
                span(id := s"order-$name-value"))
            else
              div(id := s"order-$name-field", cls := "OrderStatistics-field")(
                span(id := s"order-$name-value"),
                "\u00a0 ",
                span(id := "OrderStatistics-refresh", cls := "glyphicon glyphicon-refresh"))),
      div(clear.left))

  private def javascript =
    s"jQuery(function() { startOrderStatisticsChangedListener('${pathQuery.toUriPath}') });"
}

object OrderStatisticsWidget extends OrderStatisticsWidget(PathQuery.All)
