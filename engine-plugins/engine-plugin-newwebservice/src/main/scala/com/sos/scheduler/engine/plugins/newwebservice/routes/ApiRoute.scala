package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.base.utils.ScalaUtils.RichThrowable
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.completeWithError
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.routes.ApiRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.agent.AgentRoute
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoute
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.FrontEndRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerOverviewHtmlPage.implicits.schedulerOverviewToHtmlPage
import scala.concurrent.ExecutionContext
import scala.util.control.NonFatal
import spray.http.StatusCodes._
import spray.routing.Directives._
import spray.routing.{ExceptionHandler, RejectionHandler, Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait ApiRoute
extends AgentRoute
with AnyFileBasedRoute
with CommandRoute
with EventRoute
with JobRoute
with JobChainRoute
with LogRoute
with FrontEndRoute
with OrderRoute
with ProcessClassRoute
with TaskRoute {

  protected def client: DirectSchedulerClient
  protected def prefixLog: PrefixLog
  protected implicit def executionContext: ExecutionContext
  protected implicit def actorRefFactory: ActorRefFactory

  protected final def apiRoute: Route =
    handleExceptions(ApiExceptionHandler) {
      handleRejections(ApiRejectionHandler) {
        dontCache {
          pathPrefix("event") {
            eventRoute
          } ~
          specializedFileBasedRoute ~
          anyFileBasedRoute ~
          otherApiRoute
        } ~
        pathPrefix("agent") {
          agentRoute
        }
      }
    } ~
    pathPrefix("frontend") {
      frontEndRoute
    }

  private def specializedFileBasedRoute: Route =
    pathPrefix("order") {
      orderRoute
    } ~
    pathPrefix("jobChain") {
      jobChainRoute
    } ~
    pathPrefix("job") {
      jobRoute
    } ~
    pathPrefix("processClass") {
      processClassRoute
    }

  private def otherApiRoute =
    pathPrefix("command") {
      commandRoute
    } ~
    pathPrefix("task") {
      taskRoute
    } ~
    pathPrefix("scheduler") {
      pathEnd {
        parameter("return") {
          case "log" ⇒ logRoute(prefixLog)
          case _ ⇒ reject
        }
      }
    } ~
    pathEndElseRedirect(webServiceContext) {
      get {
        completeTryHtml(client.overview)
      }
    }
}

object ApiRoute {
  private val logger = Logger(getClass)

  private val ApiExceptionHandler = {
    val ErrorCodePattern = """(SCHEDULER|SOS)-\d+ .*""".r
    ExceptionHandler {
      case NonFatal(t) ⇒
        // These are an internally used web services, we expose internal error messages
        val message =
          if (ErrorCodePattern.pattern.matcher(t.getMessage).matches) {
            logger.debug(t.toString, t)
            t.getMessage
          } else {
            logger.warn(t.toString, t)
            t.toSimplifiedString
          }
        completeWithError(BadRequest, message)
    }
  }

  private val ApiRejectionHandler = RejectionHandler {
    case ValidationRejection(message, _) :: _ ⇒ completeWithError(BadRequest, message)
  }
}
