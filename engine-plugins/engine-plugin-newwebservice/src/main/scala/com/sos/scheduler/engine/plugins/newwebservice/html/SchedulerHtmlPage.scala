package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.Scheduler.buildVersion
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import java.time.Instant.now
import java.time.format.DateTimeFormatter.ISO_LOCAL_DATE
import java.time.format.DateTimeFormatterBuilder
import java.time.temporal.ChronoField._
import java.time.{Instant, OffsetDateTime}
import scala.language.implicitConversions

/**
  * @author Joacim Zschimmer
  */
trait SchedulerHtmlPage extends HtmlPage {

  protected val schedulerOverview: SchedulerOverview
  protected def title: String
  protected def headlineSuffix = title
  private val rootUri = "/jobscheduler/master/api"
  private val webjarsUri = "/jobscheduler/master/webjars"

  protected def page(innerBody: xml.NodeSeq): xml.Node =
    <html lang="en">
      {head}
      {body(innerBody)}
    </html>

  protected def head =
    <head>
      <meta http-equiv="X-UA-Compatible" content="IE=edge"/>
      <meta name="viewport" content="width=device-width, initial-scale=1"/>
      <title>{s"$title · ${schedulerOverview.schedulerId}"}</title>
      <link rel="stylesheet" href={s"$webjarsUri/bootstrap/3.3.6/css/bootstrap.min.css"} integrity="sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7"/>
      <style type="text/css">{css}</style>
    </head>

  protected def body(innerBody: xml.NodeSeq) =
    <body>
      <div class="container">
        {pageHeader}
        {innerBody}
      </div>
    </body>

  protected def pageHeader =
    <div class="gray-box" style="margin-bottom: 20px">
      <div style="float: right"><a href="javascript:window.location.href = window.location.href" class="inherit-markup">{localDateTimeWithZoneToHtml(now)}</a></div>
      <div style="color: grey">
        <a href={rootUri} class="inherit-markup">{
          s"JobScheduler $buildVersion Master"
        }</a>
        {s"· PID ${schedulerOverview.pid} · ${schedulerOverview.state}"}
      </div>
    </div>
    <h3>{
      val prefix = <a href={rootUri} class="inherit-markup">{schedulerOverview.schedulerId}</a>
      emptyToNone(headlineSuffix) match {
        case None ⇒ prefix
        case Some(suffix) ⇒ prefix ++  " · " ++ suffix
      }
    }</h3>

  //<link rel="stylesheet" href="/jobscheduler/master/webjars/bootstrap/3.3.6/css/bootstrap-theme.min.css" integrity="sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r"/>

  private def css = """
body {
  color: black;
}
th {
  font-weight: normal;
}
.gray-box {
  background-color: #f5f5f5;
  border-radius = 4px;
  padding: 1px 5px 0 5px;
}
a.inherit-markup {
  color: inherit;
}
"""
}

object SchedulerHtmlPage {
  def fileBasedStateToHtml(fileBasedState: FileBasedState) =
    <span class={fileBasedStateToBootstrapTextClass(fileBasedState)}>{fileBasedState}</span>

  def fileBasedStateToBootstrapTextClass(o: FileBasedState) = o match {
    case FileBasedState.active ⇒ "text-success"
    case FileBasedState.incomplete ⇒ "text-danger"
    case FileBasedState.undefined ⇒ "text-danger"
    case _ ⇒ ""
  }

  def instantWithDurationToHtml(instant: Instant): xml.Node =
    if (instant == Instant.EPOCH) xml.Text("now")
    else xml.Text(s"""${localDateTimeToHtml(instant)} (${(now - instant).pretty})""")

  private val LocalDateTimeFormatter = new DateTimeFormatterBuilder()
    .append(ISO_LOCAL_DATE)
    .appendLiteral(' ')
    .appendValue(HOUR_OF_DAY, 2)
    .appendLiteral(':')
    .appendValue(MINUTE_OF_HOUR, 2)
    .appendLiteral(':')
    .appendValue(SECOND_OF_MINUTE, 2)
    .appendLiteral('.')
    .appendValue(MILLI_OF_SECOND, 3)
    .toFormatter

  private val LocalDateTimeWithZoneFormatter = new DateTimeFormatterBuilder()
    .append(LocalDateTimeFormatter)
    .appendLiteral(' ')
    .appendOffsetId
    .toFormatter

  def localDateTimeToHtml(instant: Instant): xml.Node =
    xml.Text(LocalDateTimeFormatter.format(OffsetDateTime.ofInstant(instant, Scheduler.DefaultZoneId)))

  def localDateTimeWithZoneToHtml(instant: Instant): xml.Node =
    xml.Text(LocalDateTimeWithZoneFormatter.format(OffsetDateTime.ofInstant(instant, Scheduler.DefaultZoneId)))
}
