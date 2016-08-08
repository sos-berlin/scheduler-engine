package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.common.sprayutils.YamlPrinter
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import scala.concurrent.{ExecutionContext, Future}
import spray.http.Uri
import spray.json._

/**
  * @author Joacim Zschimmer
  */
object YamlHtmlPage {

  object implicits {
    import scala.language.implicitConversions

    implicit def toYamlHtmlPage[A: RootJsonWriter](implicit client: SchedulerClient) = new ToHtmlPage[A] {
      def apply(pageUri: Uri, webServiceContext: WebServiceContext)(value: A)
        (implicit ec: ExecutionContext)
      = {
        val yamlFuture = Future { YamlPrinter(value.toJson) }
        for (schedulerOverview ← client.overview;
             yaml ← yamlFuture) yield
          new StringHtmlPage(yaml, pageUri, webServiceContext, schedulerOverview)
      }
    }
  }
}
