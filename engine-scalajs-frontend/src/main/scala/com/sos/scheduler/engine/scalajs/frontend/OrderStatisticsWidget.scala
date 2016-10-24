package com.sos.scheduler.engine.scalajs.frontend

import com.sos.scheduler.engine.scalajs.frontend.Utils._
import org.scalajs.dom.html
import scala.scalajs.concurrent.JSExecutionContext.Implicits.queue
import scala.scalajs.js
import scala.scalajs.js.JSON
import scala.scalajs.js.annotation.JSExport
import scala.util.{Failure, Success}
import scalatags.JsDom.all._

/**
  * @author Joacim Zschimmer
  */
@JSExport
final class OrderStatisticsWidget {

  private val element = div.render

  @JSExport
  def start(): html.Element = {
    showOrderStatistics(0)
    element
  }

  private def showOrderStatistics(after: EventId = 0): Unit = {
    getDynamic(s"api/event/?return=OrderStatisticsChanged&timeout=60&after=$after") onComplete {
      case Success(responseSnapshot) ⇒
        val events = responseSnapshot.eventSnapshots.asInstanceOf[js.Array[js.Dynamic]]
        element.textContent = JSON.stringify(events.head.orderStatistics)
        val lastEventId: EventId = events.last.eventId.asInstanceOf[Double].toLong
        // delay
        showOrderStatistics(lastEventId)
      case Failure(stat) ⇒
        // for (() <- timer(5.s)) showOrderStatistics()
    }
  }
}
