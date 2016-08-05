package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.jobchain.PathQueryHttp.directives.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.TextHtmlPage.implicits._
import scala.concurrent._
import spray.httpx.marshalling.ToResponseMarshallable.isMarshallable
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
    get {
      parameterMap { parameterMap ⇒
        val returnType = parameterMap.get("return")
        pathQuery {
          case single: PathQuery.SinglePath ⇒ singleJobRoute(single.as[JobPath], returnType)
          case query ⇒ multipleJobsRoute(returnType)
        }
      }
    }

  private def singleJobRoute(jobPath: JobPath, returnType: Option[String]): Route =
    returnType match {
      case Some("JobOverview") | None ⇒ completeTryHtml(client.jobOverview(jobPath))
      //case Some("JobDetails") | None ⇒ completeTryHtml(client.jobDetails(jobPath))
      case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }

  private def multipleJobsRoute(returnType: Option[String]): Route =
    returnType match {
      case Some("JobOverview") | None ⇒ completeTryHtml(client.jobOverviews)
      case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }
}
