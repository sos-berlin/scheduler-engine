package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, StandingOrderSubsystem}
import com.sos.scheduler.engine.plugins.newwebservice.JsonProtocol._
import com.sos.scheduler.engine.plugins.newwebservice.MyDirectives.toughComplete
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait WebServices {
  protected def orderSubsystem: OrderSubsystem
  protected def standingOrderSubsystem: StandingOrderSubsystem
  protected def configuration: NewWebServicePluginConfiguration
  protected def scheduler: Scheduler
  protected def fileBasedSubsystemRegister: FileBasedSubsystem.Register
  protected implicit def actorRefFactory: ActorRefFactory
  protected implicit def executionContext = actorRefFactory.dispatcher

  //private val tickDuration = Duration.ofMillis(Try { context.system.settings.config.getDuration("akka.scheduler.tick-duration", TimeUnit.MILLISECONDS) } getOrElse Int.MaxValue)

  final def route: Route =
    (decompressRequest() & compressResponseIfRequested(())) {
      get {
        pathPrefix("new" / "master" / "api") {  // Nicht "jobscheduler", weil sonst Jettys GzipFilter dazwischenfunkt und jpeg falsch liefert. com.sos.scheduler.engine.plugins.jetty.configuration.Config.contextPath stripPrefix "/"
          apiRoute ~ testRoute
        }
      }
    }

  private def apiRoute: Route =
    get {
      pathEndOrSingleSlash {
        detach(()) {
          complete {
            scheduler.overview
          }
        }
      } ~
      pathPrefix("order") {
        (pathSingleSlash | pathPrefix("OrderOverview") & pathSingleSlash) {
          detach(()) {
            complete {
              orderSubsystem.orderOverviews
            }
          }
        }
      }
      /*~
      pathPrefix("subsystems") {
        subsystemsRoute
      }
      */
    }

  private def subsystemsRoute: Route =
    pathEnd {
      complete {
        fileBasedSubsystemRegister.descriptions map { _.fileBasedType.cppName }
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

  private def testRoute: Route =
    if (configuration.testMode)
      pathPrefix("test") {
        path("OutOfMemoryError") {
          toughComplete {
            throw new OutOfMemoryError
          }
        } ~
        path("ERROR-500") {
          complete {
            sys.error("ERROR " * 10)
          }
        }
      }
    else
      reject
}
