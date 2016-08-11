package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class StringHtmlPage(
  protected val snapshot: Snapshot[String],
  protected val pageUri: Uri,
  implicit protected val uris: SchedulerUris,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  def wholePage = htmlPage(
    pre(cls := "ContentBox ContentBox-single")(
      StringFrag(snapshot.value)))
}
