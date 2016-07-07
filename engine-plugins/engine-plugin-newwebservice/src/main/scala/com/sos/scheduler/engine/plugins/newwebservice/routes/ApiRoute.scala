package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.XmlString
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.plugins.newwebservice.json.JsonProtocol._
import scala.concurrent.ExecutionContext
import spray.http.StatusCodes.TemporaryRedirect
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait ApiRoute extends OrderRoute {

  protected def client: DirectSchedulerClient
  protected def fileBasedSubsystemRegister: FileBasedSubsystem.Register
  protected implicit def executionContext: ExecutionContext

  protected final def apiRoute: Route =
    pathPrefix("api") {
      pathEnd {
        get {
          complete(client.overview)
        }
      } ~
      pathEndOrSingleSlash {
        detach(()) {
          complete {
            client.overview
          }
        }
      } ~
      (pathPrefix("command") & pathEnd) {
        post {
          entity(as[XmlString]) { case XmlString(xmlString) ⇒
           complete(client.executeXml(xmlString) map XmlString.apply)
          }
        }
      } ~
      pathPrefix("order") {
        (pathEnd & get) {
           redirect("order/", TemporaryRedirect)
        } ~
        orderRoute
      }
      /*~
      pathPrefix("subsystems") {
        subsystemsRoute
      }
      */
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
