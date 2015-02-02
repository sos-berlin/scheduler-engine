package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.ActorLogging
import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.JsonProtocol._
import com.sos.scheduler.engine.plugins.newwebservice.MyDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.WebServiceActor._
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import java.util.concurrent.TimeUnit
import javax.inject.Inject
import org.joda.time.Duration
import scala.util.Try
import spray.httpx.SprayJsonSupport._
import spray.routing._
import spray.routing.directives.MethodDirectives._

final class WebServiceActor @Inject private(
  configuration: NewWebServicePluginConfiguration,
  injector: Injector)
extends HttpServiceActor with ActorLogging {

  //private implicit def executionContext = actorRefFactory.dispatcher  // For Futures and actorSystem.schedule()

  private val tickDuration = new Duration(Try { context.system.settings.config.getDuration("akka.scheduler.tick-duration", TimeUnit.MILLISECONDS) } getOrElse Int.MaxValue)

  // Die Scheduler-Objekte stehen beim Start noch nicht bereit (es käme zu einem Deadlock, weil wir in einem anderem Thread sind).
  private lazy val schedulerConfiguration = injector.apply[SchedulerConfiguration]
  private lazy val scheduler = injector.apply[Scheduler]
  private lazy val fileBasedSubsystemRegister = injector.apply[FileBasedSubsystem.Register]

  override def receive = runRoute(route)

  private def route: Route =
    (readOnlyMethods | post) {
      compressResponseIfRequested(()) {
        pathPrefix(BasePath) {
          get {
            contextPathRoute
          }
        }
      }
    }

  private def contextPathRoute: Route =
    pathPrefix("engine") {
      engineRoute
    } ~
    (if (configuration.testMode) testRoute else reject)

  private def testRoute: Route =
    readOnlyMethods {
      path("OutOfMemoryError") {
        toughComplete {
          throw new OutOfMemoryError
        }
      } ~
      path("ERROR") {
        complete {
          sys.error("ERROR " * 10)
        }
      }
    }

  private def engineRoute: Route = getEngineRoute

  private def getEngineRoute =
    get {
      pathPrefix("main") {
        path("overview") {
          detach(()) {
            complete {
              scheduler.overview
            }
          }
        }
      } ~
      pathPrefix("subsystems") {
        pathEndOrSingleSlash {
          complete {
            fileBasedSubsystemRegister.descriptions map { _.fileBasedType.cppName }
          }
        } ~
        pathPrefix(Segment) { subsystemName ⇒
          val subsystem = fileBasedSubsystemRegister.subsystem(subsystemName)
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
      }
    }
}

object WebServiceActor {
  private val BasePath = "new" // Nicht "jobscheduler", weil sonst Jettys GzipFilterdazwischenfunkt und jpeg falsch liefert. com.sos.scheduler.engine.plugins.jetty.configuration.Config.contextPath stripPrefix "/"

  private def readOnlyMethods = options | get | head
}
