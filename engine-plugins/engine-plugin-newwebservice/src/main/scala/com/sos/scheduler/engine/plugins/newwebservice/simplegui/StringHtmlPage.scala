package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.data.compounds.SchedulerResponse
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class StringHtmlPage(
  protected val response: SchedulerResponse[String],
  protected val pageUri: Uri,
  implicit protected val webServiceContext: WebServiceContext,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  def wholePage = htmlPage(
    pre(cls := "ContentBox ContentBox-single")(
      StringFrag(response.content)))
}
