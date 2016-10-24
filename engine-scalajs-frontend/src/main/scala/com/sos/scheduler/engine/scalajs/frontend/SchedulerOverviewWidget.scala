package com.sos.scheduler.engine.scalajs.frontend

import com.sos.scheduler.engine.scalajs.frontend.Utils._
import com.sos.scheduler.engine.scalajs.frontend.data.SchedulerOverview
import org.scalajs.dom.html
import scala.scalajs.concurrent.JSExecutionContext.Implicits.queue
import scala.scalajs.js.annotation.JSExport
import scalatags.JsDom.all._

@JSExport
final class SchedulerOverviewWidget {

  @JSExport
  def start(): html.Element = {
    val element = div.render
    for (dyn ← getDynamic("api");
         overview = SchedulerOverview(dyn)) {
      element.textContent = schedulerOverviewToText(overview)
    }
    element
  }

  private def schedulerOverviewToText(overview: SchedulerOverview): String = {
    import overview.{httpPort, pid, schedulerId, startedAt, state, udpPort, version}
    s"$version $schedulerId, started at $startedAt, PID $pid, TCP port $httpPort, UDP port $udpPort $state"
  }
}
