package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.plugins.newwebservice.common.SprayUtils.passIf
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait AllRoutes extends ApiRoute with WebjarsRoute with JocCompatibleRoute with TestRoute {

  protected implicit def actorRefFactory: ActorRefFactory
  protected final def executionContext = actorRefFactory.dispatcher
  protected final val webServiceContext = new WebServiceContext(htmlEnabled = webjarsExists)

  final def route: Route =
    (decompressRequest() & compressResponseIfRequested(())) {
      // Prefix "jobscheduler" equals JettyPlugin's context name. This lets Jetty's GzipFilter scramble JPEG.
      pathPrefix("jobscheduler") {
        pathPrefix("master") {
          masterRoute
        } ~
        jocCompatibleRoute
      }
    }

  private def masterRoute: Route =
    pathPrefix("api") {
      apiRoute
    } ~
    pathPrefix("webjars") {
      webjarsRoute
    } ~
    pathPrefix("cpp") {
      cppHttpRoute
    } ~
    (passIf(configuration.testMode) & pathPrefix("TEST")) {
      testRoute
    }
}
