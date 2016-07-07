package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait WebServices extends ApiRoute with TestRoute {

  protected implicit def actorRefFactory: ActorRefFactory
  protected def executionContext = actorRefFactory.dispatcher

  final def route: Route =
    (decompressRequest() & compressResponseIfRequested(())) {
      pathPrefix("jobscheduler" / "master") {  // Prefix "jobscheduler" equals JettyPlugin's context name. This lets Jetty's GzipFilter scramble JPEG.
        apiRoute ~
        pathPrefix("TEST") {
          testRoute
        }
      }
    }
}
