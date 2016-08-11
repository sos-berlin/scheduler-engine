package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.common.sprayutils.YamlPrinter
import com.sos.scheduler.engine.data.event.Snapshot
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

    implicit def jsonToYamlHtmlPage[A: RootJsonWriter](implicit client: SchedulerClient, webServiceContext: WebServiceContext, ec: ExecutionContext) =
      ToHtmlPage[Snapshot[A]] { (snapshot, pageUri) ⇒
        val yamlFuture = Future { snapshot map { o ⇒ YamlPrinter(o.toJson) } }
        for (Snapshot(schedulerOverview) ← client.overview;
             yaml ← yamlFuture) yield
          new StringHtmlPage(yaml, pageUri, webServiceContext.uris, schedulerOverview)
      }
  }
}
