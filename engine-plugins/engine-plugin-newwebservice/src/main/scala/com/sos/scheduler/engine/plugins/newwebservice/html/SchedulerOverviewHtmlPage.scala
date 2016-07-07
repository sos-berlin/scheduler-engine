package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
final class SchedulerOverviewHtmlPage private(protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  import schedulerOverview._

  override def title = "JobScheduler"
  override def headlineSuffix = ""

  def node = page {
    <p style="margin-bottom: 30px">
      Started at {instantWithDurationToHtml(startInstant)} · {
        ((tcpPort map { o ⇒ s" TCP port $o" }) ++
        (udpPort map { o ⇒ s" UDP port $o" }) ++
        Some(s"PID $pid") ++
        Some(state))
        .mkString(" · ")
    }</p>
    <p>
      <a href="api/order/">Orders</a>,
      <a href="api/order/?suspended=true">suspended</a>,
      <a href="api/order/?sourceType=adHoc">ad-hoc</a>
    </p>
  }
}

object SchedulerOverviewHtmlPage {
  import scala.language.implicitConversions

  implicit def apply(overview: SchedulerOverview)(implicit ec: ExecutionContext): Future[HtmlPage] =
    Future.successful(new SchedulerOverviewHtmlPage(overview))
}
