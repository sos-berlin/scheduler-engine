package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.Scheduler.DefaultZoneId
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.routes.WebjarsRoute
import java.time.Instant.now
import java.time.format.DateTimeFormatter.ISO_LOCAL_DATE
import java.time.format.{DateTimeFormatter, DateTimeFormatterBuilder}
import java.time.temporal.ChronoField._
import java.time.{Instant, OffsetDateTime}
import scala.language.implicitConversions
import scalatags.Text.all._
import scalatags.Text.{TypedTag, tags2}
import scalatags.text.Frag

/**
  * @author Joacim Zschimmer
  */
trait SchedulerHtmlPage extends HtmlPage {

  protected val schedulerOverview: SchedulerOverview
  protected def title: String
  protected def headlineSuffix = title
  protected val webServiceContext: WebServiceContext

  protected final lazy val uris = SchedulerUris(webServiceContext.baseUri)

  protected def htmlPage(innerBody: Frag*): TypedTag[String] =
    html(lang := "en")(
      pageHead,
      pageBody(innerBody :_*))

  protected def pageHead =
    head(
      meta(httpEquiv := "X-UA-Compatible", content := "IE=edge"),
      meta(name := "viewport", content := "width=device-width, initial-scale=1"),
      tags2.title(s"$title · ${schedulerOverview.schedulerId}"),
      raw(webServiceContext.toStylesheetLinkHtml(WebjarsRoute.BootstrapCss).toString),
      tags2.style(`type` := "text/css")(css))

  protected def pageBody(innerBody: Frag*) =
    body(
      pageHeader,
      div(cls := "container", width := "100%")(
        innerBody))

  protected def pageHeader = {
    import schedulerOverview.{pid, state, version}
    div(cls := "pageHeader")(
      div(float.right)(a(href := "javascript:window.location.href = window.location.href", cls := "inherit-markup")(localDateTimeWithZoneToHtml(now))),
      div(color.gray)(
        a(href := uris.overview, cls := "inherit-markup")(s"JobScheduler $version Master"),
        s" · PID $pid · $state"))
  }

  protected def headline =
    h1(cls := "headLine") {
      val prefix = a(href := uris.overview, cls := "inherit-markup")(
        img("width".attr := 40, "height".attr := 40, verticalAlign.`text-bottom`,
          src := s"${uris.resolvePathUri("api/frontend/images/job_scheduler_rabbit_circle_60x60.gif")}"),
        s" JobScheduler '${schedulerOverview.schedulerId.string}'")
      emptyToNone(headlineSuffix) match {
        case None ⇒ prefix
        case Some(suffix) ⇒ prefix :: StringFrag(s" · $suffix") :: Nil
      }
    }

  //<link rel="stylesheet" href="/jobscheduler/master/webjars/bootstrap/3.3.6/css/bootstrap-theme.min.css" integrity="sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r"/>

  protected def css = """
body {
  color: black;
  font-size: 13px;
}
div.pageHeader {
  margin-bottom: 10px;
  padding: 1px 5px 0 5px;
  border-radius = 2px;
  background-color: #f5f5f5;
}
h1.headLine {
  margin-top: 0;
}
h2 {
  margin-top: 60px;
  font-size: 24px;
}
h3 {
  margin: 20px 0 10px 0;
  font-size: 13px;
  font-weight: bold;
}
th {
  font-weight: normal;
}
a.inherit-markup {
  color: inherit;
}
label {
  font-weight: inherit;
}
span.time-extra {
 font-size: 11px;
 color: #808080;
}
"""
}

object SchedulerHtmlPage {
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

  def instantWithDurationToHtml(instant: Instant): List[Frag]  =
    if (instant == Instant.EPOCH)
      StringFrag("immediately") :: Nil
    else
      localDateTimeToHtml(instant) ::
        span(cls := "time-extra")(s".${formatTime(LocalMillisFormatter, instant)} (${(now - instant).pretty})") ::
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

  def localDateTimeToHtml(instant: Instant) = StringFrag(formatTime(LocalDateTimeFormatter, instant))

  def localDateTimeWithZoneToHtml(instant: Instant) =
    raw(formatTime(LocalDateTimeWithZoneFormatter, instant))

  private def formatTime(formatter: DateTimeFormatter, instant: Instant) =
    formatter.format(OffsetDateTime.ofInstant(instant, DefaultZoneId))
}
