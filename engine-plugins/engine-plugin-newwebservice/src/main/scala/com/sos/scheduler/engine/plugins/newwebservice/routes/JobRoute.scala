package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.google.common.base.Splitter
import com.sos.scheduler.engine.client.web.common.HasViewCompanionDirectives.viewReturnParameter
import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.data.job.{JobDetailed, JobOverview, JobPath, JobState, JobView}
import com.sos.scheduler.engine.data.queries.{JobQuery, PathQuery}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.JobRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes.{events, singleKeyEvents}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import java.util.regex.Pattern
import scala.collection.JavaConversions._
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

      case Some("History") ⇒
        parameter("limit".as[Int]) { limit ⇒
          complete(
            client.jobsHistory(jobPath, limit))
        }

      case _ ⇒
        viewReturnParameter(JobView, default = JobDetailed) { implicit viewCompanion ⇒
          val future = client.job(jobPath)
          completeTryHtml(future)
        } ~
        singleKeyEvents[AnyEvent](jobPath)
    }

  private def multipleJobsRoute(pathQuery: PathQuery): Route =
    parameter("state" ? "") { state ⇒
      val isInJobState: JobState ⇒ Boolean = CommaSplitter.split(state).map(JobState.valueOf).toSet match {
        case o if o.isEmpty ⇒ _ ⇒ true
        case o ⇒ o.contains
      }
      val query = JobQuery(pathQuery, isInJobState)
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
              case KeyedEvent(jobPath: JobPath, _) ⇒ query.pathQuery.matches(jobPath)
              case _ ⇒ false
            })
      }
    }
}

object JobRoute {
  private val CommaSplitter = Splitter.on(Pattern.compile(",")).trimResults.omitEmptyStrings
}
