package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.{completeTryHtml, _}
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.routes.ApiRoute._
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
trait ApiRoute extends JobChainRoute with OrderRoute with CommandRoute {

  protected def client: DirectSchedulerClient
  //protected def fileBasedSubsystemRegister: FileBasedSubsystem.Register
  protected implicit def executionContext: ExecutionContext

  protected final def apiRoute: Route =
    respondWithHeader(`Cache-Control`(`max-age`(0), `no-store`, `no-cache`)) {
      handleExceptions(ApiExceptionHandler) {
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
        }
        /*~
        pathPrefix("subsystems") {
          subsystemsRoute
        }
        */
      }
    }

  /*
  private def subsystemsRoute: Route =
    pathEnd {
      get {
        complete(fileBasedSubsystemRegister.descriptions map { _.fileBasedType.cppName })
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
              subsystem.fileBased(subsystem.description.stringToPath(fileBasedPath)).details
            }
          }
        }
      }
    }
    */
}

object ApiRoute {
  private val logger = Logger(getClass)

  private val ApiExceptionHandler = ExceptionHandler {
    // This is an internal API, so we expose internal error messages !!!
    case e: CppException if e.getCode == "SCHEDULER-161" ⇒ complete((NotFound, e.getMessage))
    case e: IllegalArgumentException ⇒ complete((BadRequest, e.getMessage))
    case e: RuntimeException ⇒ complete((BadRequest, e.getMessage))
    case NonFatal(t) ⇒
      logger.debug(t.toString, t)
      val message = t match {
        case _: IllegalArgumentException | _: RuntimeException ⇒ t.getMessage
        case _ ⇒ t.toString
      }
      complete((BadRequest, message))
  }
}
