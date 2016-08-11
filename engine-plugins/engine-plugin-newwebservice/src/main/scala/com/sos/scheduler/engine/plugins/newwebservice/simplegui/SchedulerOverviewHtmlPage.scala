package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.data.compounds.SchedulerResponse
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.instantWithDurationToHtml
import scala.concurrent.{ExecutionContext, Future}
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SchedulerOverviewHtmlPage private(
  protected val response: SchedulerResponse[SchedulerOverview],
  protected val pageUri: Uri,
  protected val webServiceContext: WebServiceContext)
extends SchedulerHtmlPage {

  protected val schedulerOverview = response.content
  import schedulerOverview._

  def wholePage =
    htmlPage(
      p(marginBottom := "30px")(
        s"Started at ",
        instantWithDurationToHtml(startedAt),
        " · ",
        ( (httpPort map { o ⇒ s" HTTP port $o" }) ++
          (udpPort map { o ⇒ s" UDP port $o" }) ++
          Some(s"PID $pid") ++
          Some(state))
          .mkString(" · ")),
      form(action := "api/command", method := "get")(
        span(cls := "input-group input-group-sm")(
          span(cls := "input-group-addon")(
            "XML command: "),
          input(`type` := "text", autofocus, name := "command", value := "s", placeholder := "For example: show_state", cls := "form-control"),
          " ",
          span(cls := "input-group-btn")
            (button(`type` := "submit", cls := "btn btn-primary")(
              "Execute")))))
}

object SchedulerOverviewHtmlPage {
  object implicits {
    import scala.language.implicitConversions

    implicit object schedulerOverviewToHtmlPage extends ToHtmlPage[SchedulerResponse[SchedulerOverview]] {
      def apply(pageUri: Uri, webServiceContext: WebServiceContext)(response: SchedulerResponse[SchedulerOverview])
        (implicit executionContext: ExecutionContext)
      =
        Future.successful(new SchedulerOverviewHtmlPage(response, pageUri, webServiceContext))
    }
  }
}
