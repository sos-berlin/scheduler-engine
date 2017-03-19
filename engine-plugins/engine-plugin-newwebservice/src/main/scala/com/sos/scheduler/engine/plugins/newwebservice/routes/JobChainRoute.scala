package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.jobscheduler.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.jobscheduler.data.event.{AnyEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.{JobChainQuery, PathQuery}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent._
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait JobChainRoute extends EventRoutes {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: SchedulerWebServiceContext
  protected implicit def executionContext: ExecutionContext

  final def jobChainRoute: Route =
    getRequiresSlash {
      parameter("return".?) { returnType ⇒
        pathQuery(JobChainPath) {
          case single: PathQuery.SinglePath ⇒ singleJobChainRoute(single.as[JobChainPath], returnType)
          case query ⇒ multipleJobChainsRoute(JobChainQuery(query), returnType)
        }
      }
    }

  private def singleJobChainRoute(jobChainPath: JobChainPath, returnType: Option[String]): Route =
    returnType match {
      case Some("JobChainOverview") ⇒ completeTryHtml(client.jobChainOverview(jobChainPath))
      case Some("JobChainDetailed") | None ⇒ completeTryHtml(client.jobChainDetailed(jobChainPath))
      case Some(o) ⇒ singleKeyEvents[AnyEvent](jobChainPath)
    }

  private def multipleJobChainsRoute(query: JobChainQuery, returnType: Option[String]): Route =
    returnType match {
      case Some("JobChainOverview") | None ⇒
        completeTryHtml(client.jobChainOverviewsBy(query))
      case Some(_) ⇒
        events[Event](
          predicate = {
            case KeyedEvent(jobChainPath: JobChainPath, _) ⇒ query.pathQuery.matches(jobChainPath)
            case _ ⇒ false
          })
    }
}
