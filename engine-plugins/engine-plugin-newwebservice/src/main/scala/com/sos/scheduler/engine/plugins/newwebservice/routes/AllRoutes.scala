package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.plugins.newwebservice.common.SprayUtils.passIf
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.htmlPreferred
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import spray.http.StatusCodes.TemporaryRedirect
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
      pathEndOrSingleSlash {
        htmlPreferred(webServiceContext) {
          redirect("jobscheduler/joc", TemporaryRedirect)
        }
      } ~
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
    (passIf(configuration.testMode) & pathPrefix("TEST")) {
      testRoute
    }
}
