package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.data.event.Stamped
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class StringHtmlPage(
  protected val stampedString: Stamped[String],
  protected val pageUri: Uri,
  implicit protected val uris: SchedulerUris,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  protected def eventId = stampedString.eventId

  def wholePage = htmlPage(
    div(
      "EventId: ", eventId.toString, " (", SchedulerHtmlPage.eventIdToLocalHtml(eventId), ")"),
    pre(cls := "ContentBox ContentBox-single")(
      stampedString.value))
}
