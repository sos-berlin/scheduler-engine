package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.client.web.common.PathQueryHttp.directives.pathQuery
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, EventSeq}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.kernel.event.OrderStatisticsChangedSource
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait OrderEventRoute extends HasCloser {

  protected def orderStatisticsChangedSource: OrderStatisticsChangedSource
  protected implicit def client: SchedulerOverviewClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  protected def orderPathEventRoute: Route =
    pathQuery(JobChainPath) { query ⇒
      withEventParameters {
        case EventParameters("OrderStatisticsChanged", afterEventId, limit) ⇒
          completeTryHtml[EventSeq[Seq, AnyKeyedEvent]] {
            for (snapshot ← orderStatisticsChangedSource.whenOrderStatisticsChanged(after = afterEventId, query))
              yield nestIntoSeqSnapshot(snapshot)
          }
        case _ ⇒ reject
      }
    }
}
