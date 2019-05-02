package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.common.QueryHttp.jobChainQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.{JobChainQuery, PathQuery}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes.{events, singleKeyEvents}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent._
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait JobChainRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  final def jobChainRoute: Route =
    getRequiresSlash(webServiceContext) {
      parameter("return".?) { returnType ⇒
        jobChainQuery { query ⇒
          query.pathQuery match {
            case single: PathQuery.SinglePath ⇒ singleJobChainRoute(single.as[JobChainPath], returnType)  // other query arguments are ignored
            case _ ⇒ multipleJobChainsRoute(query, returnType)
          }
        }
      }
    }

  private def singleJobChainRoute(jobChainPath: JobChainPath, returnType: Option[String]): Route =
    returnType match {
      case Some("JobChainOverview") ⇒ completeTryHtml(client.jobChainOverview(jobChainPath))
      case Some("JobChainDetailed") | None ⇒ completeTryHtml(client.jobChainDetailed(jobChainPath))
      case Some(_) ⇒ singleKeyEvents[AnyEvent](jobChainPath)
    }

  private def multipleJobChainsRoute(query: JobChainQuery, returnType: Option[String]): Route =
    returnType match {
      case Some("JobChainOverview") | None ⇒
        completeTryHtml(client.jobChainOverviewsBy(query))
      case Some("JobChainDetailed") =>
        completeTryHtml(client.jobChainDetailedBy(query))
      case Some(o) ⇒
        events[Event](
          predicate = {
            case KeyedEvent(jobChainPath: JobChainPath, _) ⇒ query.pathQuery.matches(jobChainPath)
            case _ ⇒ false
          })
    }
}
