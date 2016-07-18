package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.common.sprayutils.YamlPrinter
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import scala.concurrent.{ExecutionContext, Future}
import scalatags.Text.all._
import spray.json._

/**
  * @author Joacim Zschimmer
  */
final class TextHtmlPage(protected val schedulerOverview: SchedulerOverview, text: String)
  (implicit protected val webServiceContext: WebServiceContext)
extends SchedulerHtmlPage {

  protected def title = "JobScheduler"
  override def headlineSuffix = ""

  def scalatag = htmlPage(
    headline,
    pre(
      StringFrag(text)))
}

object TextHtmlPage {
  object implicits {
    import scala.language.implicitConversions

    implicit def toHtmlPage[A: RootJsonWriter](value: A)(
      implicit
        webServiceContext: WebServiceContext,
        client: DirectSchedulerClient,
        ec: ExecutionContext): Future[HtmlPage]
    =
      for (overview ← client.overview) yield
        new TextHtmlPage(overview, YamlPrinter(value.toJson))
  }
}
