package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.job.{JobDescription, JobOverview, JobPath}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent._
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}

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

  private def singleJobRoute(jobPath: JobPath) =
    parameter("return" ? "JobOverview") {
      case "JobOverview" ⇒ completeTryHtml(client.job[JobOverview](jobPath))
      case "JobDescription" ⇒ complete(client.job[JobDescription](jobPath))
      case o ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }

  private def multipleJobsRoute(query: PathQuery) =
    parameter("return" ? "JobOverview") {
      case "JobOverview" ⇒ completeTryHtml(client.jobs[JobOverview](query))
      case "JobDescription" ⇒ complete(client.jobs[JobDescription](query))
      case o ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }
}
