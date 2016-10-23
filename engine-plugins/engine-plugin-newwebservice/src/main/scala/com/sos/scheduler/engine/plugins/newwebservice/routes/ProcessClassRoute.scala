package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.{ProcessClassClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.processclass.{ProcessClassDetailed, ProcessClassOverview, ProcessClassPath}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.toHtmlPage
import scala.concurrent._
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait ProcessClassRoute {

  protected implicit def client: ProcessClassClient with SchedulerOverviewClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  final def processClassRoute: Route =
    getRequiresSlash(webServiceContext) {
      pathQuery(ProcessClassPath) {
        case single: PathQuery.SinglePath ⇒
          parameter("return" ? "ProcessClassDetailed") { returnType ⇒
            singleProcessClassRoute(single.as[ProcessClassPath], returnType)
          }
        case query ⇒
          parameter("return" ? "ProcessClassOverview") { returnType ⇒
            multipleProcessClassRoute(query, returnType)
          }
      }
    }

  private def singleProcessClassRoute(processClassPath: ProcessClassPath, returnType: String): Route =
    returnType match {
      case "ProcessClassOverview" ⇒ completeTryHtml(client.processClass[ProcessClassOverview](processClassPath))
      case "ProcessClassDetailed" ⇒ completeTryHtml(client.processClass[ProcessClassDetailed](processClassPath))
      case o ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }

  private def multipleProcessClassRoute(query: PathQuery, returnType: String): Route =
    returnType match {
      case "ProcessClassOverview" ⇒ completeTryHtml(client.processClasses[ProcessClassOverview](query))
      case "ProcessClassDetailed" ⇒ completeTryHtml(client.processClasses[ProcessClassDetailed](query))
      case o ⇒ reject(ValidationRejection(s"Not allowed return=$o"))
    }
}
