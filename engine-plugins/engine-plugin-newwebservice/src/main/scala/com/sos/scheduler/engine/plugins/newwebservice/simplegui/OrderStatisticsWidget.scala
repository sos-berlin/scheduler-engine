package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import scalatags.Text.all._
import scalatags.text.Frag

/**
  * @author Joacim Zschimmer
  */
object OrderStatisticsWidget {

  private val fieldGroups = List(
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
      "fileOrder"))

  val html: Frag =
    div(id := "OrderStatistics")(marginBottom := 1.em)(
      for (nameGroup ← fieldGroups) yield
        div(cls := "OrderStatistics-fieldGroup")(
          for (name ← nameGroup) yield
            div(id := s"order-$name-field", cls := "OrderStatistics-field")(
              s"$name:\u2009",
              span(id := s"order-$name-value")("\u2007\u2007\u2007\u2007"))),
      " ",
      span(id := "OrderStatistics-refresh", cls := "glyphicon glyphicon-refresh", float.left, position.relative, top := 2.px, marginRight := 8.px),
      div(clear.left))
}
