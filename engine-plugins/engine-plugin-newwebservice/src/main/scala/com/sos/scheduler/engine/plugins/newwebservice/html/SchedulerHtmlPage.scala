package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.client.web.SchedulerUris
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
  protected def title: String = "JobScheduler"
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
      raw(s"<style type='text/css'>$css</style>"))

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
            localDateTimeWithZoneToHtml(now))),
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
              src := s"${ uris.resolvePathUri("api/frontend/images/job_scheduler_rabbit_circle_60x60.gif") }")),
            span(" JobScheduler"))),
        ul(cls := "nav navbar-nav nav-pills")(
          li(role := "presentation", cls := (if (this.isInstanceOf[SchedulerOverviewHtmlPage]) "active" else ""))(
            a(href := uris.resolvePathUri("api/").toString)(s"'${schedulerOverview.schedulerId.string}'")),
          li(role := "presentation", cls := (if (this.isInstanceOf[OrdersHtmlPage]) "active" else ""))(
            a(href := uris.resolvePathUri("api/order/").toString)("Orders")),
          li(role := "presentation")(
            a(href := uris.resolvePathUri("api/jobChain/").toString)("Job chains")),
          li(role := "presentation")(
            a(href := uris.resolvePathUri("api/job/").toString)("Jobs")),
          li(role := "presentation")(
            a(href := uris.resolvePathUri("api/processClass/").toString)("Process classes")))))

  //<link rel="stylesheet" href="/jobscheduler/master/webjars/bootstrap/3.3.6/css/bootstrap-theme.min.css" integrity="sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r"/>

  protected def css = """
body {
  background-color: #f8f8f8;
  color: black;
  font-size: 13px;
}
.container {
  padding: 0 7px 20px 7px;
}
div.PageHeader {
  margin-bottom: 4px;
  padding: 1px 5px 0 5px;
}
h1 {
  font-size: 30px;
}
h1.headLine {
  margin-top: 0;
  padding: 0px 5px;
}
h2 {
  margin-top: 0;
  font-size: 24px;
}
h3 {
  margin: 20px 0 10px 0;
  font-size: 18px;
}
.table {
  margin-bottom: 0;
}
.table-condensed>tbody>tr>td,
.table-condensed>tbody>tr>th,
.table-condensed>tfoot>tr>td,
.table-condensed>tfoot>tr>th,
.table-condensed>thead>tr>td,
.table-condensed>thead>tr>th {
  border-top: 1px solid #eee;
  padding-top: 3px;
  padding-bottom: 3px;
}
.table>thead>tr>th {
  border-top: 0;
  font-weight: normal;
}
.MiniTable>tbody>tr>td,
.MiniTable>tbody>tr>th,
.MiniTable>tfoot>tr>td,
.MiniTable>tfoot>tr>th,
.MiniTable>thead>tr>td,
.MiniTable>thead>tr>th {
  padding-right: 5px;
}
a.inherit-markup {
  color: inherit;
}
label {
  font-weight: inherit;
}
span.time-extra {
 font-size: 11px;
 //color: @text-muted;
}
.ContentBox {
  margin-top: 40px;
  border: 2px solid #eee;
  border-top: 1px solid #eee;
  background-color: white;
}
.Padded {
  padding: 0 5px;
}
.container-fluid {
  background-color: white;
}
"""
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
