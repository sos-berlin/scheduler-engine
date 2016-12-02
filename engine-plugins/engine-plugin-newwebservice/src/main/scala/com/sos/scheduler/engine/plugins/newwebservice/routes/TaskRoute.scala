package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.job.{TaskEvent, TaskId, TaskKey}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.job.TaskSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes.singleKeyEvents
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent.ExecutionContext
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
    getRequiresSlash(webServiceContext) {
      path(IntNumber) { taskNumber ⇒
        val taskId = TaskId(taskNumber)
        parameter("return" ? "TaskOverview") {
          case "log" ⇒ logRoute(taskSubsystem.task(taskId).log)
          case "TaskOverview" ⇒ completeTryHtml(client.taskOverview(taskId))
          case _ ⇒
            //singleKeyEvents(classOf[TaskEvent], ??? : TaskKey)
            reject
        }
      }
    }
}
