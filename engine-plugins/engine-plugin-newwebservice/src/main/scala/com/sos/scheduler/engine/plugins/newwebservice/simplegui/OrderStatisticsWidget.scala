package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import scalatags.Text.all._
import scalatags.text.Frag

/**
  * @author Joacim Zschimmer
  */
object OrderStatisticsWidget {

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

  val html: Frag =
    div(id := "OrderStatistics")(
      for (nameGroup ← FieldGroups) yield
        div(cls := "OrderStatistics-fieldGroup")(
          for (name ← nameGroup) yield
            if (name != TimestampName)
              div(id := s"order-$name-field", cls := "OrderStatistics-field")(
                s"$name:\u2009",
                span(id := s"order-$name-value"))
            else
              div(id := s"order-$name-field")(
                span(id := s"order-$name-value"),
                "\u00a0 ",
                span(id := "OrderStatistics-refresh", cls := "glyphicon glyphicon-refresh"))),
      div(clear.left))
}
