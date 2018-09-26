package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.common.HasViewCompanionDirectives.viewReturnParameter
import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.data.job.{JobDetailed, JobOverview, JobPath, JobView}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes.{events, singleKeyEvents}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent._
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait JobRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  final def jobRoute: Route =
    getRequiresSlash(webServiceContext) {
      pathQuery(JobPath) {
        case single: PathQuery.SinglePath ⇒ singleJobRoute(single.as[JobPath])
        case query ⇒ multipleJobsRoute(query)
      }
    }

  private def singleJobRoute(jobPath: JobPath): Route =
    parameter("return".?) {
      case Some("JocOrderStatistics") ⇒
        parameter("isDistributed".as[Boolean].?) { isDistributed ⇒
          complete(
            client.jobJocOrderStatistics(jobPath, isDistributed = isDistributed))
        }

      case _ ⇒
        viewReturnParameter(JobView, default = JobDetailed) { implicit viewCompanion ⇒
          val future = client.job(jobPath)
          completeTryHtml(future)
        } ~
        singleKeyEvents[AnyEvent](jobPath)
    }

  private def multipleJobsRoute(query: PathQuery): Route =
    parameter("return".?) {
      case Some("JocOrderStatistics") ⇒
        parameter("isDistributed".as[Boolean].?) { isDistributed ⇒
          complete(
            client.jobsJocOrderStatistics(query, isDistributed = isDistributed))
        }

      case _ ⇒
        viewReturnParameter(JobView, default = JobOverview) { implicit viewCompanion ⇒
          val future = client.jobs(query)
          completeTryHtml(future)
        } ~
        events[Event](
          predicate = {
            case KeyedEvent(jobPath: JobPath, _) ⇒ query.matches(jobPath)
            case _ ⇒ false
          })
    }
}
