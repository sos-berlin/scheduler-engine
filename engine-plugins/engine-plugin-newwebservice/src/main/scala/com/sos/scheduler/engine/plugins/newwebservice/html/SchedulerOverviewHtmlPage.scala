package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerHtmlPage._
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
final class SchedulerOverviewHtmlPage private(
  protected val schedulerOverview: SchedulerOverview,
  protected val webServiceContext: WebServiceContext)
extends SchedulerHtmlPage {

  import schedulerOverview._

  override def title = "JobScheduler"
  override def headlineSuffix = ""

  def node = page {
    headline ++
    <p style="margin-bottom: 30px">
      Started at {instantWithDurationToHtml(startInstant)} · {
        ((tcpPort map { o ⇒ s" TCP port $o" }) ++
        (udpPort map { o ⇒ s" UDP port $o" }) ++
        Some(s"PID $pid") ++
        Some(state))
        .mkString(" · ")
    }</p>
    <p>
      <a href="api/order/">Orders</a>
      <br/>
      <a href="api/jobChain/">Job chains</a>
    </p>
  }
}

object SchedulerOverviewHtmlPage {
  import scala.language.implicitConversions

  implicit def toHtml(overview: SchedulerOverview)(implicit webServiceContext: WebServiceContext, ec: ExecutionContext): Future[HtmlPage] =
    Future.successful(new SchedulerOverviewHtmlPage(overview, webServiceContext))
}
