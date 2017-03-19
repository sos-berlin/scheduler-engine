package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.common.sprayutils.YamlPrinter
import com.sos.jobscheduler.common.sprayutils.html.HtmlDirectives.ToHtmlPage
import com.sos.jobscheduler.data.event.Stamped
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext
import scala.concurrent.{ExecutionContext, Future}
import spray.json._

/**
  * @author Joacim Zschimmer
  */
object YamlHtmlPage {

  object implicits {
    import scala.language.implicitConversions

    implicit def jsonToYamlHtmlPage[A: RootJsonWriter](implicit client: SchedulerOverviewClient, webServiceContext: SchedulerWebServiceContext, ec: ExecutionContext) =
      ToHtmlPage[Stamped[A]] { (stamped, pageUri) ⇒
        val yamlFuture = Future { stamped map { o ⇒ YamlPrinter(o.toJson) } }
        for (Stamped(_, schedulerOverview) ← client.overview;
             yaml ← yamlFuture) yield
          new StringHtmlPage(yaml, pageUri, webServiceContext, schedulerOverview)
      }
  }
}
