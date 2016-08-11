package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.compounds.SchedulerResponse
import com.sos.scheduler.engine.data.event.EventId
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.Scheduler.DefaultZoneId
import com.sos.scheduler.engine.plugins.newwebservice.html.{HtmlPage, WebServiceContext}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder.{toAsyncScriptHtml, toCssLinkHtml}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.WebjarsRoute.NeededWebjars
import java.time.Instant.now
import java.time.format.DateTimeFormatter.ISO_LOCAL_DATE
import java.time.format.{DateTimeFormatter, DateTimeFormatterBuilder}
import java.time.temporal.ChronoField._
import java.time.{Instant, OffsetDateTime}
import scala.language.implicitConversions
import scalatags.Text.all._
import scalatags.Text.{TypedTag, tags2}
import scalatags.text.Frag
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
trait SchedulerHtmlPage extends HtmlPage {

  protected def response: SchedulerResponse[Any]
  protected val schedulerOverview: SchedulerOverview
  protected def title: String = "JobScheduler"
  protected val webServiceContext: WebServiceContext
  protected def pageUri: Uri

  import webServiceContext.uris

  protected def htmlPage(innerBody: Frag*): TypedTag[String] =
    html(lang := "en")(
      head(htmlHeadFrags),
      pageBody(innerBody :_*))

  protected def htmlHeadFrags: Vector[Frag] = {
    val includer = new HtmlIncluder(uris)
    Vector(
      meta(httpEquiv := "X-UA-Compatible", content := "IE=edge"),
      meta(name := "viewport", content := "width=device-width, initial-scale=1"),
      tags2.title(s"$title · ${schedulerOverview.schedulerId}")) ++
    (NeededWebjars flatMap includer.webjarsToHtml) ++
    (cssLinks map toCssLinkHtml) ++
    (scriptLinks map toAsyncScriptHtml)
  }

  protected def cssLinks: Vector[Uri] = Vector(uris / "api/frontend/common/common.css")
  protected def scriptLinks: Vector[Uri] = Vector()

  protected def pageBody(innerBody: Frag*) =
    body(
      div(cls := "container", width := "100%")(
        pageHeader,
        innerBody))

  protected def pageHeader = {
    import schedulerOverview.{pid, state, version}
    List(
      div(cls := "PageHeader")(
        div(float.right)(
          a(href := "javascript:window.location.href = window.location.href", cls := "inherit-markup")(
            eventIdToLocalHtml(response.eventId),
            " ",
            span(cls := "time-extra")(DefaultZoneId.getId))),
        div(color.gray)(
          a(href := uris.overview, cls := "inherit-markup")(
            s"JobScheduler $version Master"),
          s" · PID $pid · $state")),
      navbar)
  }

  private def navbar =
    nav(cls := "navbar navbar-default")(
      div(cls := "container-fluid")(
        div(cls := "navbar-header")(
          a(cls := "navbar-brand", position.relative, top := (-9).px, href := uris.overview, whiteSpace.nowrap)(
            span(img("width".attr := 40, "height".attr := 40,
              src := uris.uriString("api/frontend/common/images/job_scheduler_rabbit_circle_60x60.gif"))),
            span(" JobScheduler"))),
        ul(cls := "nav navbar-nav nav-pills")(
          navBarTab(schedulerOverview.schedulerId.string, uris.overview),
          navBarTab("Orders"         , uris.order(OrderQuery.All, returnType = None)),
          navBarTab("Job chains"     , uris.jobChain.overviews()),
          navBarTab("Jobs"           , uris.job.overviews()),
          navBarTab("Process classes", uris.processClass.overviews()),
          navBarTab("Events"         , uris.events))))

  private def navBarTab(label: String, relativeUri: String) = {
    val uri = uris / relativeUri
    val isActive = pageUri.path == uri.path
    li(role := "presentation", cls := (if (isActive) "active" else ""))(
      a(href := uri.toString)(label))
  }
}

object SchedulerHtmlPage {
  private lazy val nav = "nav".tag[String]

  object EmptyFrag extends Frag {
    def writeTo(strb: StringBuilder) = {}
    def render = ""
  }

  def fileBasedStateToHtml(fileBasedState: FileBasedState) =
    span(cls := fileBasedStateToBootstrapTextClass(fileBasedState))(fileBasedState.toString)

  def fileBasedStateToBootstrapTextClass(o: FileBasedState) = o match {
    case FileBasedState.active ⇒ "text-success"
    case FileBasedState.incomplete ⇒ "text-danger"
    case FileBasedState.undefined ⇒ "text-danger"
    case _ ⇒ ""
  }

  def eventIdToLocalHtml(eventId: EventId): List[Frag] = {
    val instant = EventId.toInstant(eventId)
    localDateTimeToHtml(instant) ::
      span(cls := "time-extra")(s".${formatDateTime(LocalMillisFormatter, instant)}") ::
      Nil
  }

  def instantWithDurationToHtml(instant: Instant): List[Frag] =
    if (instant == Instant.EPOCH)
      StringFrag("immediately") :: Nil
    else
      localDateTimeToHtml(instant) ::
        span(cls := "time-extra")(s".${formatDateTime(LocalMillisFormatter, instant)} (${(now - instant).pretty})") ::
        Nil

  private val LocalDateTimeFormatter = new DateTimeFormatterBuilder()
    .append(ISO_LOCAL_DATE)
    .appendLiteral(' ')
    .appendValue(HOUR_OF_DAY, 2)
    .appendLiteral(':')
    .appendValue(MINUTE_OF_HOUR, 2)
    .appendLiteral(':')
    .appendValue(SECOND_OF_MINUTE, 2)
    .toFormatter

  private val LocalMillisFormatter = new DateTimeFormatterBuilder()
    .appendValue(MILLI_OF_SECOND, 3)
    .toFormatter

  private val LocalDateTimeWithZoneFormatter = new DateTimeFormatterBuilder()
    .append(LocalDateTimeFormatter)
    .appendLiteral(' ')
    .appendLiteral("<span class='time-extra'>")
    .appendOffsetId
    .appendLiteral("</span>")
    .toFormatter

  def localDateTimeToHtml(instant: Instant) = StringFrag(formatDateTime(LocalDateTimeFormatter, instant))

  def localDateTimeWithZoneToHtml(instant: Instant) =
    raw(formatDateTime(LocalDateTimeWithZoneFormatter, instant))

  private def formatDateTime(formatter: DateTimeFormatter, instant: Instant) =
    formatter.format(OffsetDateTime.ofInstant(instant, DefaultZoneId))
}
