package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.data.event.{EventId, Snapshot}
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class StringHtmlPage(
  protected val snapshot: Snapshot[String],
  eventId: EventId,
  protected val pageUri: Uri,
  implicit protected val uris: SchedulerUris,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  def wholePage = htmlPage(
    div(
      "EventId: ", eventId.toString, " (", SchedulerHtmlPage.eventIdToLocalHtml(eventId), ")"),
    pre(cls := "ContentBox ContentBox-single")(
      snapshot.value))
}
