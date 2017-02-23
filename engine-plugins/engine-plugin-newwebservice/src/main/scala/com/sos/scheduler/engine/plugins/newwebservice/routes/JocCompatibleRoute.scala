package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.sprayutils.SprayUtils.{passIf, pathSegments}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.cpp.CppHttpRoute
import java.nio.file.Path
import spray.http.StatusCodes._
import spray.http.Uri
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait JocCompatibleRoute extends CommandRoute with CppHttpRoute {

  protected def schedulerConfiguration: SchedulerConfiguration
  protected def webServiceContext: WebServiceContext

  final def jocCompatibleRoute: Route =
    pathSegments("operations_gui") {
      schedulerConfiguration.existingHtmlDirOption match {
        case Some(_) ⇒ redirect("/jobscheduler/joc", TemporaryRedirect)
        case None ⇒ jocIsNotInstalled
      }
    } ~
    pathSegments("joc") {
      schedulerConfiguration.existingHtmlDirOption match {
        case Some(dir) ⇒ jocRoute(dir)
        case None ⇒ jocIsNotInstalled
      }
    } ~
    pathSegments("engine") {
      (pathSegments("command") & pathEndOrSingleSlash) {  // SingleSlash for compatibility with JOC 1
        untypedPostCommandRoute
      }
    } ~
    pathSegments("engine-cpp") {
      cppHttpRoute
    }

  private def jocIsNotInstalled = complete(NotFound → "JOC 1 is not installed")

  private def jocRoute(directory: Path): Route =
    get {
      pathEnd {
        requestInstance { request ⇒
          passIf(request.uri.query == Uri.Query.Empty) {
            val withSlash = request.uri.copy(
              scheme = "",
              authority = Uri.Authority.Empty,
              path = Uri.Path(request.uri.path.toString + "/"))
            redirect(withSlash, TemporaryRedirect)
          }
        }
      } ~
      pathSingleSlash {
        getFromFile(directory / "index.html")
      } ~
      path("index.html") {
        requestInstance { request ⇒
          val withoutFile = request.uri.copy(
            scheme = "",
            authority = Uri.Authority.Empty,
            path = request.uri.path.reverse.tail.reverse)
          redirect(withoutFile, TemporaryRedirect)
        }
        //complete((NotFound, "Not Found"))
      } ~ {
        implicit val resolver = OurContentTypeResolver
        getFromDirectory(directory.toString)
      }
    }
}
