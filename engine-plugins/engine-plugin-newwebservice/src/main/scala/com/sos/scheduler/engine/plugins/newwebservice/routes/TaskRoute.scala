package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.jobscheduler.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.jobscheduler.common.sprayutils.SprayUtils._
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.web.common.QueryHttp.pathQuery
import com.sos.scheduler.engine.data.job.{JobPath, TaskEvent, TaskKey}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.job.TaskSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent.ExecutionContext
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait TaskRoute extends LogRoute with EventRoutes {

  protected def taskSubsystem: TaskSubsystemClient
  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: SchedulerWebServiceContext
  protected implicit def executionContext: ExecutionContext

  def taskRoute: Route =
    parameter("return" ? "TaskOverview") { returnType ⇒
      parameter("taskId".as[TaskId].?) {
        case Some(taskId) ⇒
          pathEnd {
            singleTask(taskId, returnType)
          }
        case None ⇒
          multipleTasks(returnType)
      }
    }

  private def singleTask(taskId: TaskId, returnType: String): Route =
    returnType match {
      case "log" ⇒ logRoute(taskSubsystem.task(taskId).log)  // EXPERIMENTAL. Under Windows, the file is locked !!!
      case "TaskOverview" ⇒ completeTryHtml(client.taskOverview(taskId))
      case _ ⇒
        events[TaskEvent](
          predicate = {
            case KeyedEvent(TaskKey(_, `taskId`), _) ⇒ true
            case _ ⇒ false
          })
    }

  private def multipleTasks(returnType: String): Route =
    getRequiresSlash {
      pathQuery[JobPath].apply { query ⇒
        returnType match {
          case "TaskOverview" ⇒ completeTryHtml(client.taskOverviews(query))
          case _ ⇒
            events[TaskEvent](
              predicate = {
                case KeyedEvent(TaskKey(jobPath: JobPath, _), _) ⇒ query.matches(jobPath)
                case _ ⇒ false
              })
        }
      }
    }
}
