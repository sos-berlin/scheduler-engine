package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.XmlString
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.plugins.newwebservice.common.SprayUtils.accept
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.json.JsonProtocol._
import com.sos.scheduler.engine.plugins.newwebservice.routes.ApiRoute._
import scala.concurrent.ExecutionContext
import spray.http.CacheDirectives.{`max-age`, `no-cache`, `no-store`}
import spray.http.HttpHeaders.`Cache-Control`
import spray.http.MediaTypes.`text/html`
import spray.http.StatusCodes._
import spray.routing.Directives._
import spray.routing.{ExceptionHandler, Route}

/**
  * @author Joacim Zschimmer
  */
trait ApiRoute extends JobChainRoute with OrderRoute {

  protected def client: DirectSchedulerClient
  protected def fileBasedSubsystemRegister: FileBasedSubsystem.Register
  protected implicit def executionContext: ExecutionContext

  protected final def apiRoute: Route =
    pathPrefix("api") {
      respondWithHeader(`Cache-Control`(`max-age`(0), `no-store`, `no-cache`)) {
        handleExceptions(ApiExceptionHandler) {
          (pathEnd & get) {
            completeTryHtml(client.overview)
          } ~
          (pathSingleSlash & get) {
            accept(`text/html`) {
              redirect("../api", TemporaryRedirect)
            }
          } ~
          (pathPrefix("command") & pathEnd & post) {
            entity(as[XmlString]) { case XmlString(xmlString) ⇒
             complete(client.executeXml(xmlString) map XmlString.apply)
            }
          } ~
          pathPrefix("order") {
            (pathEnd & get) {
               redirect("order/", TemporaryRedirect)
            } ~
            orderRoute
          } ~
          pathPrefix("jobChain") {
            (pathEnd & get) {
               redirect("jobChain/", TemporaryRedirect)
            } ~
            jobChainRoute
          }
          /*~
          pathPrefix("subsystems") {
            subsystemsRoute
          }
          */
        }
      }
    }

  private def subsystemsRoute: Route =
    pathEnd {
      get {
        complete(fileBasedSubsystemRegister.descriptions map { _.fileBasedType.cppName })
      }
    } /*~
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
  private val ApiExceptionHandler = ExceptionHandler {
    case e: CppException if e.getCode == "SCHEDULER-161" ⇒ complete((NotFound, e.getMessage))
    case e: CppException ⇒ complete((BadRequest, e.getMessage))
  }
}
