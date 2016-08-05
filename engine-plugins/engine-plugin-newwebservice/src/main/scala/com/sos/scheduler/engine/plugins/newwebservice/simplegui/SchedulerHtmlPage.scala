package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.Scheduler.DefaultZoneId
import com.sos.scheduler.engine.plugins.newwebservice.html.{HtmlPage, WebServiceContext}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage._
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

  protected val schedulerOverview: SchedulerOverview
  protected def title: String = "JobScheduler"
  protected val webServiceContext: WebServiceContext

  import webServiceContext.uris

  protected def htmlPage(innerBody: Frag*): TypedTag[String] =
    html(lang := "en")(
      head(htmlHeadFrags),
      pageBody(innerBody :_*))

  protected def htmlHeadFrags: Vector[Frag] =
    Vector(
      meta(httpEquiv := "X-UA-Compatible", content := "IE=edge"),
      meta(name := "viewport", content := "width=device-width, initial-scale=1"),
      tags2.title(s"$title · ${schedulerOverview.schedulerId}"),
      link(rel := "stylesheet", href := uris.uriString("api/frontend/webjars/" + WebjarsRoute.BootstrapCss))) ++
    (cssLinks map toCssLinkHtml) ++
    (scriptLinks map toAsyncScriptHtml)

  protected def cssLinks: Vector[Uri] = Vector("api/frontend/common/common.css")
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
              src := uris.uriString("api/frontend/common/images/job_scheduler_rabbit_circle_60x60.gif"))),
            span(" JobScheduler"))),
        ul(cls := "nav navbar-nav nav-pills")(
          li(role := "presentation", cls := (if (this.isInstanceOf[SchedulerOverviewHtmlPage]) "active" else ""))(
            a(href := uris.uriString("api/"))(s"'${schedulerOverview.schedulerId.string}'")),
          li(role := "presentation", cls := (if (this.isInstanceOf[OrdersHtmlPage]) "active" else ""))(
            a(href := uris.uriString("api/order/"))("Orders")),
          li(role := "presentation")(
            a(href := uris.uriString("api/jobChain/"))("Job chains")),
          li(role := "presentation")(
            a(href := uris.uriString("api/job/"))("Jobs")),
          li(role := "presentation")(
            a(href := uris.uriString("api/processClass/"))("Process classes")))))

  //<link rel="stylesheet" href="/jobscheduler/master/webjars/bootstrap/3.3.6/css/bootstrap-theme.min.css" integrity="sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r"/>

  final def toCssLinkHtml(uri: Uri) = link(rel := "stylesheet", `type` := "text/css", href := uris.uriString(uri))

  final def toAsyncScriptHtml(uri: Uri) = script(`type` := "text/javascript", src := uris.uriString(uri), "async".emptyAttr)
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
