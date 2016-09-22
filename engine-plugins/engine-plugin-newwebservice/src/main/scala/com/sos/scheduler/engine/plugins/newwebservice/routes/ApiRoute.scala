package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.base.utils.ScalaUtils.RichThrowable
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.routes.ApiRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoute
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.FrontEndRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerOverviewHtmlPage.implicits.schedulerOverviewToHtmlPage
import scala.concurrent.ExecutionContext
import scala.util.control.NonFatal
import spray.http.CacheDirectives.{`max-age`, `no-cache`, `no-store`}
import spray.http.HttpHeaders.`Cache-Control`
import spray.http.StatusCodes._
import spray.routing.Directives._
import spray.routing.{ExceptionHandler, Route}

/**
  * @author Joacim Zschimmer
  */
trait ApiRoute
extends JobChainRoute
with OrderRoute
with JobRoute
with ProcessClassRoute
with TaskRoute
with CommandRoute
with LogRoute
with EventRoute
with FrontEndRoute {

  protected def client: DirectSchedulerClient
  //protected def fileBasedSubsystemRegister: FileBasedSubsystem.Register
  protected def prefixLog: PrefixLog
  protected implicit def executionContext: ExecutionContext
  protected implicit def actorRefFactory: ActorRefFactory

  protected final def apiRoute: Route =
    respondWithHeader(`Cache-Control`(`max-age`(0), `no-store`, `no-cache`)) {
      handleExceptions(ApiExceptionHandler) {
        realApiRoute
      }
    } ~
    pathPrefix("frontend") {
      frontEndRoute
    }

  private def realApiRoute =
    handleExceptions(exceptionHandler) {
      pathEndElseRedirect(webServiceContext) {
        get {
          completeTryHtml(client.overview)
        }
      } ~
      (pathPrefix("command") & pathEnd) {
        commandRoute
      } ~
      pathPrefix("order") {
        orderRoute
      } ~
      pathPrefix("jobChain") {
        testSlash(webServiceContext) {
          jobChainRoute
        }
      } ~
      pathPrefix("job") {
        testSlash(webServiceContext) {
          jobRoute
        }
      } ~
      pathPrefix("processClass") {
        testSlash(webServiceContext) {
          processClassRoute
        }
      } ~
      pathPrefix("task") {
        testSlash(webServiceContext) {
          taskRoute
        }
      } ~
      pathPrefix("scheduler") {
        pathEnd {
          parameter("return") {
            case "log" ⇒ logRoute(prefixLog)
            case _ ⇒ reject
          }
        }
      } ~
      pathPrefix("event") {
        testSlash(webServiceContext) {
          eventRoute
        }
      }
      /*~
      pathPrefix("subsystems") {
        subsystemsRoute
      }
      */
    }


  /*
  private def subsystemsRoute: Route =
    pathEnd {
      get {
        complete(fileBasedSubsystemRegister.companions map { _.fileBasedType.cppName })
      }
    } ~
    pathPrefix(Segment) { subsystemName ⇒
      val subsystem = fileBasedSubsystemRegister.subsystem(FileBasedType.fromCppName(subsystemName))
      pathEnd {
        complete {
          subsystem.filebasedOverviews
        }
      } ~
      path("overview") {
        detach(()) {
          complete {
            subsystem.overview
          }
        }
      } ~
      path("paths") {
        detach(()) {
          complete {
            subsystem.paths map { _.string }
          }
        }
      } ~
      path("fileBased" / "detailed") {
        parameter("path") { fileBasedPath ⇒
          detach(()) {
            complete {
              subsystem.fileBased(subsystem.companion.stringToPath(fileBasedPath)).detailed
            }
          }
        }
      }
    }
    */

  private val exceptionHandler = ExceptionHandler {
    case e: CppException if e.getMessage startsWith "SCHEDULER-161" ⇒ complete((NotFound, e.getMessage))
  }
}

object ApiRoute {
  private val logger = Logger(getClass)

  private val ApiExceptionHandler = ExceptionHandler {
    // This is an internal API, so we expose internal error messages !!!
    case e: CppException if e.getCode == "SCHEDULER-161" ⇒ complete((NotFound, e.getMessage))
//    case e: IllegalArgumentException ⇒ complete((BadRequest, e.toSimplifiedString))
//    case e: RuntimeException ⇒ complete((BadRequest, e.toSimplifiedString))
    case NonFatal(t) ⇒
      logger.warn(t.toString, t)
      complete((BadRequest, t.toSimplifiedString))
  }
}
