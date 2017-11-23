package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.{passIf, passSome, pathSegments}
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.htmlPreferred
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.AllRoutes._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.WebjarsRoute
import spray.http.StatusCodes.TemporaryRedirect
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait AllRoutes extends ApiRoute with WebjarsRoute with JocCompatibleRoute with TestRoute {

  protected implicit def actorRefFactory: ActorRefFactory
  protected final val webServiceContext = new WebServiceContext(htmlEnabled = webjarsExists)
  private val guiPath: Option[String] = schedulerConfiguration.existingHtmlDirOption match {
    case Some(_) ⇒ Some(Joc1Path)
    case None ⇒ webServiceContext.htmlEnabled option ExperimentalGuiPath
  }

  final def route: Route =
    decompressRequest() {    // & compressResponseIfRequested(())) {  // JOC-352 beim Lesen von JOC 1 prototype.js: IllegalArgumentException: Cannot create MessageChunk with empty data
      pathEndOrSingleSlash {
        redirectToDefaultGui
      } ~
      pathSegments("jobscheduler") {
        pathSegments("master") {
          masterRoute
        } ~
        pathEndOrSingleSlash {
          redirectToDefaultGui
        } ~
          jocCompatibleRoute
      }
    }

  private def redirectToDefaultGui: Route =
    htmlPreferred(webServiceContext) {
      passSome(guiPath) { path ⇒
        redirect(path, TemporaryRedirect)
      }
    }

  private def masterRoute: Route =
    pathEndOrSingleSlash {
      htmlPreferred(webServiceContext) {
        redirect("/jobscheduler/master/api", TemporaryRedirect)
      }
    } ~
    pathSegments("api") {
      apiRoute
    } ~
    (passIf(configuration.testMode) & pathSegments("TEST")) {
      testRoute
    }
}

object AllRoutes {
  private val Joc1Path = "/jobscheduler/joc/"
  private val ExperimentalGuiPath = "/jobscheduler/master"
}
