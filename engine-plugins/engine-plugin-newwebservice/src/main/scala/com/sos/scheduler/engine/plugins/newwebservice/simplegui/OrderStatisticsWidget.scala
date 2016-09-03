package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.joinHtml
import scalatags.Text.all._
import scalatags.text.Frag

/**
  * @author Joacim Zschimmer
  */
object OrderStatisticsWidget {

  private val fieldNames = List(
    "total",
    "notPlanned",
    "planned",
    "pending",
    "running",
    "inTask",
    "inProcess",
    "setback",
    "suspended",
    "blacklisted",
    "permanent",
    "fileOrder")

  val html: Frag =
    div(id := "OrderStatistics")(marginBottom := 1.em)(
      for (name ← fieldNames) yield
        div(id := s"order-$name-field", cls := "OrderStatistics-field", float.left, width := 15.ex, marginTop := 2.px, marginRight := 1.ex, whiteSpace.nowrap)(
          s"$name:\u2009",
          span(id := s"order-$name-value")("\u2007\u2007\u2007\u2007")),
      " ",
      span(id := "OrderStatistics-refresh", cls := "glyphicon glyphicon-refresh", float.left, position.relative, top := 2.px, marginRight := 8.px),
      div(clear.left))
}
