package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.jobchain.PathQueryHttp.directives.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.JobChainQuery
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.TextHtmlPage.implicits._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import scala.concurrent._
import spray.http.StatusCodes.NotFound
import spray.httpx.marshalling.ToResponseMarshallable.isMarshallable
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.{ExceptionHandler, Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait JobChainRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  final def jobChainRoute: Route =
    get {
      parameterMap { parameterMap ⇒
        val returnType = parameterMap.get("return")
        pathQuery { query ⇒
          query.reduce[JobChainPath] match {
            case jobChainPath: JobChainPath ⇒ singleJobChainRoute(jobChainPath, returnType)
            case _ ⇒ multipleJobChainsRoute(JobChainQuery.Standard(query), returnType)
          }
        }
      }
    }

  private def singleJobChainRoute(jobChainPath: JobChainPath, returnType: Option[String]): Route =
    handleExceptions(ExceptionHandler {
      case e: CppException if e.getMessage startsWith "SCHEDULER-161" ⇒ complete((NotFound, e.getMessage))
    }) {
      returnType match {
        case Some("JobChainOverview") ⇒ completeTryHtml(client.jobChainOverview(jobChainPath))
        case Some("JobChainDetails") | None ⇒ completeTryHtml(client.jobChainDetails(jobChainPath))
        case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
      }
    }

  private def multipleJobChainsRoute(query: JobChainQuery, returnType: Option[String]): Route =
    returnType match {
      case Some("JobChainOverview") | None ⇒ completeTryHtml(client.jobChainOverviewsBy(query))
      case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }
}
