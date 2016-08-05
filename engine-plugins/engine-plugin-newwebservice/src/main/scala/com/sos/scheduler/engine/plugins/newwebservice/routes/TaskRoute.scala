package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.job.TaskSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.toYamlHtmlPage
import scala.concurrent.ExecutionContext
import spray.httpx.marshalling.ToResponseMarshallable.isMarshallable
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait TaskRoute extends LogRoute {

  protected def taskSubsystem: TaskSubsystemClient
  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  def taskRoute: Route =
    path(IntNumber) { taskNumber ⇒
      val taskId = TaskId(taskNumber)
      parameter("return") {
        case "log" ⇒ logRoute(taskSubsystem.task(taskId).log)
      } ~
        completeTryHtml(taskSubsystem.taskOverview(taskId))
    }
}
