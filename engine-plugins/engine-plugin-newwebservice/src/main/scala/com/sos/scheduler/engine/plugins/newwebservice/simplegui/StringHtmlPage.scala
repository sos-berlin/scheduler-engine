package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.data.event.Stamped
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class StringHtmlPage(
  protected val stampedString: Stamped[String],
  protected val pageUri: Uri,
  protected val webServiceContext: SchedulerWebServiceContext,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  private implicit val uris = webServiceContext.uris
  protected def eventId = stampedString.eventId

  def wholePage = htmlPage(
    div(
      "EventId: ", eventId.toString, " (", SchedulerHtmlPage.eventIdToLocalHtml(eventId), ")"),
    pre(cls := "ContentBox ContentBox-single")(
      stampedString.value))
}
