package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.{ProcessClassClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.client.web.common.HasViewCompanionDirectives.viewReturnParameter
import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.jobscheduler.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.processclass.{ProcessClassDetailed, ProcessClassOverview, ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent._
import spray.json.DefaultJsonProtocol._
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
          val processClassPath = single.as[ProcessClassPath]
          viewReturnParameter(ProcessClassView, default = ProcessClassDetailed) { implicit viewCompanion ⇒
            val future = client.processClass(processClassPath)
            completeTryHtml(future)
          }
        case query ⇒
          viewReturnParameter(ProcessClassView, default = ProcessClassOverview) { implicit viewCompanion ⇒
            val future = client.processClasses(query)
            completeTryHtml(future)
          }
      }
    }
}
