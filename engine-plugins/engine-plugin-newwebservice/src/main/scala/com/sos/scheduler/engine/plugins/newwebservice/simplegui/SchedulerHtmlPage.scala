package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.jobscheduler.data.event.{Event, EventId, ReverseEventRequest, Snapshot}
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.JobOverview
import com.sos.scheduler.engine.data.processclass.ProcessClassOverview
import com.sos.scheduler.engine.data.queries.{OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.Scheduler.DefaultZoneId
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.{joinHtml, seqFrag}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder.{toCssLinkHtml, toScriptHtml, toVersionedUriPath}
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
  protected def pageTitle: String = "JobScheduler"
  protected val uris: SchedulerUris
  protected def pageUri: Uri

  private val includer = new HtmlIncluder(uris)

  protected def htmlPage(innerBody: Frag*): TypedTag[String] =
    html(lang := "en")(
      head(htmlHeadFrag),
      body(pageBody(innerBody :_*)))

  protected def htmlHeadFrag: Frag =
    seqFrag(
      meta(httpEquiv := "X-UA-Compatible", content := "IE=edge"),
      meta(name := "viewport", content := "width=device-width, initial-scale=1"),
      tags2.title(pageTitle, " · ", schedulerOverview.schedulerId.string),
      css,
      javascript,
      link(rel := "icon", "sizes".attr := "64x64", `type` := "image/vnd.microsoft.icon",
        href := (uris / "api/frontend/common/images/jobscheduler.ico").toString))

  private def css: Frag =
    (NeededWebjars map includer.cssHtml) ++
      (for (o ← cssPaths) yield toCssLinkHtml(uris / o))

  private def javascript: Frag =
    (NeededWebjars map includer.javascriptHtml) ++
      (for (o ← scriptPaths) yield toScriptHtml(uris / o))

  protected def cssPaths: Vector[String] = CssPaths
  protected def scriptPaths: Vector[String] = ScriptPaths

  protected def pageBody(innerBody: Frag*): Frag =
    seqFrag(
      navbar,
      div(cls := "container", width := "100%")(
        innerBody))

  private def timestampHtml: Frag =
    a(href := "javascript:window.location.href = window.location.href", cls := "inherit-markup", title := "Click or press Enter to refresh")(
      span(id := "refresh", cls := "glyphicon glyphicon-refresh", position.relative, top := 2.px, marginRight := 8.px),
      eventIdToLocalHtml(snapshot.eventId, withSubseconds = false),
      " ",
      span(cls := "time-extra")(DefaultZoneId.getId))

  private def navbar: Frag =
    nav(cls := "navbar navbar-default navbar-static-top")(
      div(cls := "container-fluid")(
        div(cls := "navbar-header")(
          uncollapseButton,
          brand),
        div(cls := "collapse navbar-collapse")(
          menu,
          div(cls := "navbar-text navbar-right", marginBottom := 0)(
            table(position.relative, top := (-7).px)(
              tbody(
                tr(td(textAlign.right, timestampHtml)),
                tr(td(textAlign.right, letterSpacing := 1.px)("EXPERIMENTAL API VIEWER"))))))))

  private def brand: Frag =
    a(cls := "navbar-brand", position.relative, top := (-9).px, whiteSpace.nowrap, href := uris.overview)(
      table(
        tbody(
          tr(
            td(rowspan := 2, paddingRight := 1.ex)(
              img("width".attr := 40, "height".attr := 40, alt := "Rabbit",
                src := uris.uriString(RabbitPicturePath))),
            td(
              span(" JobScheduler \u00a0'", schedulerOverview.schedulerId.string, "'"))),
          tr(
            td(fontSize := 13.px)(
              versionAndStateHtml)))))

  private def versionAndStateHtml: Frag = {
    import schedulerOverview.{state, version}
    joinHtml(" · ")(List(
      s"v$version",
      span(whiteSpace.nowrap)(s"$state")))
  }

  private def menu: Frag =
    ul(cls := "nav navbar-nav")(
      navBarTab("Orders"         , uris.order(OrderQuery.All, returnType = None)),
      navBarTab("Job chains"     , uris.jobChain.overviews()),
      navBarTab("Jobs"           , uris.job[JobOverview](PathQuery.All)),
      navBarTab("Process classes", uris.processClass.views[ProcessClassOverview](PathQuery.All)),
      navBarTab("Events"         , uris.events(ReverseEventRequest[Event](limit = 1000))))

  private def uncollapseButton: Frag =
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

  private val CssPaths = Vector(
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/common/common.css")))
  private val ScriptPaths = Vector(
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/common/common.js")))
  private val RabbitPicturePath =
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/common/images/job_scheduler_rabbit_circle_60x60.gif"))

  def fileBasedStateToHtml(fileBasedState: FileBasedState): Frag =
    span(cls := fileBasedStateToBootstrapTextClass(fileBasedState))(fileBasedState.toString)

  def fileBasedStateToBootstrapTextClass(o: FileBasedState): String =
    o match {
      case FileBasedState.active ⇒ "text-success"
      case FileBasedState.incomplete ⇒ "text-danger"
      case FileBasedState.undefined ⇒ "text-danger"
      case _ ⇒ ""
    }

  def midnightInstant: Instant =
    Instant.ofEpochSecond(LocalDate.now(SchedulerHtmlPage.OurZoneId).toEpochDay * 24*3600)

  def eventIdToLocalHtml(eventId: EventId, withDateBefore: Instant = Instant.MAX, withSubseconds: Boolean = true): Frag = {
    val instant = EventId.toInstant(eventId)
    seqFrag(
      instantToHtml(instant, if (instant >= withDateBefore) LocalTimeFormatter else LocalDateTimeFormatter),
      withSubseconds option subsecondsToHtml(instant))
  }

  def subsecondsToHtml(instant: Instant): Frag =
    span(cls := "time-extra")(".", formatDateTime(instant, LocalMillisFormatter))

  def instantWithDurationToHtml(instant: Instant): Frag =
    if (instant == Instant.EPOCH)
      "immediately"
    else
      seqFrag(
        instantToHtml(instant, LocalDateTimeFormatter),
        span(cls := "time-extra")(".", formatDateTime(instant, LocalMillisFormatter), " (", (now - instant).pretty), ")")

  def shortInstantWithDurationToHtml(instant: Instant): Frag =
    if (instant == Instant.EPOCH)
      "immediately"
    else
      seqFrag(
        instantToHtml(instant, ShortLocalTimeFormatter),
        span(cls := "time-extra")(" (", (now - instant).pretty), ")")

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

  private val ShortLocalTimeFormatter = new DateTimeFormatterBuilder()
    .appendValue(HOUR_OF_DAY, 2)
    .appendLiteral(':')
    .appendValue(MINUTE_OF_HOUR, 2)
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

  def localDateTimeWithZoneToHtml(instant: Instant): Frag =
    raw(formatDateTime(instant, LocalDateTimeWithZoneFormatter))

  private def formatDateTime(instant: Instant, formatter: DateTimeFormatter): String =
    formatter.format(OffsetDateTime.ofInstant(instant, OurZoneId))
}
