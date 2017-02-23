package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.common.sprayutils.YamlPrinter
import com.sos.jobscheduler.data.event.Snapshot
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import scala.concurrent.{ExecutionContext, Future}
import spray.json._

/**
  * @author Joacim Zschimmer
  */
object YamlHtmlPage {

  object implicits {
    import scala.language.implicitConversions

    implicit def jsonToYamlHtmlPage[A: RootJsonWriter](implicit client: SchedulerOverviewClient, webServiceContext: WebServiceContext, ec: ExecutionContext) =
      ToHtmlPage[Snapshot[A]] { (snapshot, pageUri) ⇒
        val yamlFuture = Future { snapshot map { o ⇒ YamlPrinter(o.toJson) } }
        for (Snapshot(eventId, schedulerOverview) ← client.overview;
             yaml ← yamlFuture) yield
          new StringHtmlPage(yaml, eventId, pageUri, webServiceContext.uris, schedulerOverview)
      }
  }
}
