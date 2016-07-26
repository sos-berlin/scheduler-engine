package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.base.exceptions.PublicException
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.routes.ApiRoute._
import scala.concurrent.ExecutionContext
import scala.util.control.NonFatal
import spray.http.CacheDirectives.{`max-age`, `min-fresh`, `no-cache`, `no-store`}
import spray.http.HttpHeaders.`Cache-Control`
import spray.http.StatusCodes._
import spray.routing.Directives._
import spray.routing.{ExceptionHandler, Route}

/**
  * @author Joacim Zschimmer
  */
trait ApiRoute extends JobChainRoute with OrderRoute with CommandRoute with LogRoute {

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
    pathEndElseRedirect(webServiceContext) {
      get {
        completeTryHtml(client.overview)
      }
    } ~
    (pathPrefix("command") & pathEnd) {
      commandRoute
    } ~
    pathPrefix("order") {
      eatSlash(webServiceContext) {
        orderRoute
      }
    } ~
    pathPrefix("jobChain") {
      eatSlash(webServiceContext) {
        jobChainRoute
      }
    } ~
    pathPrefix("scheduler") {
      path("log") {
        logRoute(prefixLog)
      }
    }
    /*~
    pathPrefix("subsystems") {
      subsystemsRoute
    }
    */

  private def frontEndRoute =
    get {
      respondWithHeader(`Cache-Control`(`min-fresh`(60))) {
        getFromResourceDirectory(FrontendResourceDirectory.path)
      }
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
      path("fileBased" / "details") {
        parameter("path") { fileBasedPath ⇒
          detach(()) {
            complete {
              subsystem.fileBased(subsystem.companion.stringToPath(fileBasedPath)).details
            }
          }
        }
      }
    }
    */
}

object ApiRoute {
  private val logger = Logger(getClass)
  private val FrontendResourceDirectory = JavaResource("com/sos/scheduler/engine/plugins/newwebservice/frontend")

  private val ApiExceptionHandler = ExceptionHandler {
    // This is an internal API, so we expose internal error messages !!!
    case e: CppException if e.getCode == "SCHEDULER-161" ⇒ complete((NotFound, e.getMessage))
    case e: IllegalArgumentException ⇒ complete((BadRequest, e.getMessage))
    case e: RuntimeException ⇒ complete((BadRequest, e.getMessage))
    case NonFatal(t) ⇒
      logger.debug(t.toString, t)
      val message = t match {
        case _: PublicException ⇒ t.getMessage
        case _ if t.getMessage.nonEmpty ⇒ t.getMessage //.stripPrefix("java.lang.RuntimeException: ")
        case _ ⇒ t.toString
      }
      complete((BadRequest, message))
  }
}
