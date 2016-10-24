package com.sos.scheduler.engine.scalajs.frontend

import scalatags.JsDom.all._
import org.scalajs.dom.html
import scala.scalajs.js.annotation.JSExport

/**
  * @author Joacim Zschimmer
  */
@JSExport
object Run {

  @JSExport
  def run(): html.Element =
    div(
      new SchedulerOverviewWidget().start(),
      div(marginTop := 2.em),
      new OrderStatisticsWidget().start(),
      div(marginTop := 2.em),
      new StartedOrdersWidget().start())
    .render
}
