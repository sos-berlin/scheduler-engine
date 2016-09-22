package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{EventId, Snapshot}
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.Scheduler.DefaultZoneId
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.{joinHtml, seqFrag}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder.{toCssLinkHtml, toScriptHtml}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.WebjarsRoute.NeededWebjars
import java.time.Instant.now
import java.time.format.DateTimeFormatter.ISO_LOCAL_DATE
import java.time.format.{DateTimeFormatter, DateTimeFormatterBuilder}
import java.time.temporal.ChronoField._
import java.time.{Instant, LocalDate, OffsetDateTime}
import scala.language.implicitConversions
import scalatags.Text.all._
import scalatags.Text.{TypedTag, tags2}
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
trait SchedulerHtmlPage extends HtmlPage {

  protected def snapshot: Snapshot[Any]
  protected val schedulerOverview: SchedulerOverview
  protected def title: String = "JobScheduler"
  protected val uris: SchedulerUris
  protected def pageUri: Uri

  protected def htmlPage(innerBody: Frag*): TypedTag[String] =
    html(lang := "en")(
      head(htmlHeadFrags),
      pageBody(innerBody :_*))

  protected def htmlHeadFrags: Vector[Frag] = {
    val includer = new HtmlIncluder(uris)
    Vector(
      meta(httpEquiv := "X-UA-Compatible", content := "IE=edge"),
      meta(name := "viewport", content := "width=device-width, initial-scale=1"),
      tags2.title(s"$title · ${schedulerOverview.schedulerId}"),
      link(rel := "icon", "sizes".attr := "64x64", `type` := "image/vnd.microsoft.icon",
        href := (uris / "api/frontend/common/images/jobscheduler.ico").toString)) ++
    (NeededWebjars flatMap includer.webjarsToHtml) ++
    (cssLinks map toCssLinkHtml) ++
    (scriptLinks map toScriptHtml)
  }

  protected def cssLinks: Vector[Uri] = Vector(uris / "api/frontend/common/common.css")
  protected def scriptLinks: Vector[Uri] = Vector()

  protected def pageBody(innerBody: Frag*) =
    body(
      pageHeader,
      navbar,
      div(cls := "container", width := "100%")(
        innerBody))

  private def pageHeader: Frag = {
    import schedulerOverview.{pid, state, version}
    val jobSchedulerInfoHtml =
      div(
        a(href := uris.overview, cls := "inherit-markup")(
          joinHtml(" · ")(List(
            emptyToNone(schedulerOverview.schedulerId.string).toList,
            "JobScheduler"))),
        joinHtml(" · ")(List(
          " ", version, " Master",
          span(whiteSpace.nowrap)(s"PID $pid"),
          span(whiteSpace.nowrap)(s"$state"))))
    val timestampHtml =
      a(href := "javascript:window.location.href = window.location.href", cls := "inherit-markup")(
        span(id := "refresh", cls := "glyphicon glyphicon-refresh", position.relative, top := 2.px, marginRight := 8.px),
        eventIdToLocalHtml(snapshot.eventId),
        " ",
        span(cls := "time-extra")(DefaultZoneId.getId))
    div(cls := "PageHeader")(
      div(float.right, paddingLeft := 2.em)(
        timestampHtml),
      jobSchedulerInfoHtml)
  }

  private def navbar: Frag =
    nav(cls := "navbar navbar-default navbar-static-top")(
      div(cls := "container-fluid")(
        div(cls := "navbar-header")(
          uncollapseButton,
          a(cls := "navbar-brand", position.relative, top := (-9).px, href := uris.overview, whiteSpace.nowrap)(
            span(img("width".attr := 40, "height".attr := 40,
              src := uris.uriString("api/frontend/common/images/job_scheduler_rabbit_circle_60x60.gif"))),
            span(" JobScheduler"))),
        div(cls := "collapse navbar-collapse")(
          ul(cls := "nav navbar-nav ")(
            navBarTab("Orders"         , uris.order(OrderQuery.All, returnType = None)),
            navBarTab("Job chains"     , uris.jobChain.overviews()),
            navBarTab("Jobs"           , uris.job.overviews()),
            navBarTab("Process classes", uris.processClass.overviews()),
            navBarTab("Events"         , uris.events(limit = 1000, reverse = true))))))

  private def uncollapseButton =
    button(`type` := "button", cls := "navbar-toggle", data("toggle") := "collapse", data("target") := ".navbar-collapse")(
      span(cls := "icon-bar"),
      span(cls := "icon-bar"),
      span(cls := "icon-bar"))

  private def navBarTab(label: String, relativeUri: String) = {
    val uri = uris / relativeUri
    val isActive = pageUri.path == uri.path
    li(role := "presentation", isActive option { cls := "active" })(
      a(href := uri.toString)(label))
  }
}

private[simplegui] object SchedulerHtmlPage {
  private lazy val nav = "nav".tag[String]
  val OurZoneId = DefaultZoneId

  def fileBasedStateToHtml(fileBasedState: FileBasedState) =
    span(cls := fileBasedStateToBootstrapTextClass(fileBasedState))(fileBasedState.toString)

  def fileBasedStateToBootstrapTextClass(o: FileBasedState) = o match {
    case FileBasedState.active ⇒ "text-success"
    case FileBasedState.incomplete ⇒ "text-danger"
    case FileBasedState.undefined ⇒ "text-danger"
    case _ ⇒ ""
  }

  def midnightInstant = Instant.ofEpochSecond(LocalDate.now(SchedulerHtmlPage.OurZoneId).toEpochDay * 24*3600)

  def eventIdToLocalHtml(eventId: EventId, withDateBefore: Instant = Instant.MAX): Frag = {
    val instant = EventId.toInstant(eventId)
    seqFrag(
      instantToHtml(instant, if (instant >= withDateBefore) LocalTimeFormatter else LocalDateTimeFormatter),
      subsecondsToHtml(instant))
  }

  def subsecondsToHtml(instant: Instant): Frag =
    span(cls := "time-extra")(s".${formatDateTime(instant, LocalMillisFormatter)}")

  def instantWithDurationToHtml(instant: Instant): Frag =
    if (instant == Instant.EPOCH)
      "immediately"
    else
      seqFrag(
        instantToHtml(instant, LocalDateTimeFormatter),
        span(cls := "time-extra")(s".${formatDateTime(instant, LocalMillisFormatter)} (${(now - instant).pretty})"))

  private val LocalTimeFormatter = new DateTimeFormatterBuilder()
    .appendValue(HOUR_OF_DAY, 2)
    .appendLiteral(':')
    .appendValue(MINUTE_OF_HOUR, 2)
    .appendLiteral(':')
    .appendValue(SECOND_OF_MINUTE, 2)
    .toFormatter

  private val LocalDateTimeFormatter = new DateTimeFormatterBuilder()
    .append(ISO_LOCAL_DATE)
    .appendLiteral(' ')
    .append(LocalTimeFormatter)
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

  private def instantToHtml(instant: Instant, formatter: DateTimeFormatter) =
    StringFrag(formatDateTime(instant, formatter))

  def localDateTimeWithZoneToHtml(instant: Instant) =
    raw(formatDateTime(instant, LocalDateTimeWithZoneFormatter))

  private def formatDateTime(instant: Instant, formatter: DateTimeFormatter): String =
    formatter.format(OffsetDateTime.ofInstant(instant, OurZoneId))
}
