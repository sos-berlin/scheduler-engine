package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.common.PathQueryHttp.directives.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.completeWithError
import com.sos.scheduler.engine.data.processclass.{ProcessClassDetailed, ProcessClassOverview, ProcessClassPath}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent._
import spray.http.StatusCodes.BadRequest
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.directives.BasicDirectives.{mapRequestContext ⇒ _}
import spray.routing.{Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait ProcessClassRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  final def processClassRoute: Route =
    get {
      parameterMap { parameterMap ⇒
        val returnType = parameterMap.get("return")
        testSlash(webServiceContext) {
          pathQuery(ProcessClassPath) {
            case single: PathQuery.SinglePath ⇒ singleProcessClassRoute(single.as[ProcessClassPath], returnType)
            case PathQuery.All ⇒ multipleProcessClassRoute(returnType)
            case query ⇒
              completeWithError(BadRequest, "PathQuery is not yet suppported") // multipleProcessClassRoute(/*ProcessClassQuery.Standard(query), */returnType)
          }
        }
      }
    }

  private def singleProcessClassRoute(processClassPath: ProcessClassPath, returnType: Option[String]): Route =
    returnType match {
      case Some("ProcessClassOverview")        ⇒ completeTryHtml(client.processClassView[ProcessClassOverview](processClassPath))
      case Some("ProcessClassDetailed") | None ⇒ completeTryHtml(client.processClassView[ProcessClassDetailed](processClassPath))
      case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }

  private def multipleProcessClassRoute(/*query: ProcessClassQuery, */returnType: Option[String]): Route =
    returnType match {
      case Some("ProcessClassOverview") | None ⇒ completeTryHtml(client.processClasses[ProcessClassOverview])
      case Some("ProcessClassDetailed")        ⇒ completeTryHtml(client.processClasses[ProcessClassDetailed])
      case Some(o) ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }
}
