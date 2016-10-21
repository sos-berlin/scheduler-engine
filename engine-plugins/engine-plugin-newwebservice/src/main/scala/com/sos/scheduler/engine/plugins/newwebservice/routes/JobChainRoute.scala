package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.{JobChainQuery, PathQuery}
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
trait JobChainRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  final def jobChainRoute: Route =
    getRequiresSlash(webServiceContext) {
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
      case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }

  private def multipleJobChainsRoute(query: JobChainQuery, returnType: Option[String]): Route =
    returnType match {
      case Some("JobChainOverview") | None ⇒ completeTryHtml(client.jobChainOverviewsBy(query))
      case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }
}
