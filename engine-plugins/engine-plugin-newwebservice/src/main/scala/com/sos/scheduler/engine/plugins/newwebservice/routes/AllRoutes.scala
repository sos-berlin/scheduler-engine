package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.{passIf, passSome}
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

  final def route: Route =
    (decompressRequest() & compressResponseIfRequested(())) {
      pathEndOrSingleSlash {
        redirectToDefaultGui
      } ~
      pathPrefix("jobscheduler") {
        pathPrefix("master") {
          masterRoute
        } ~
        pathEndOrSingleSlash {
          redirectToDefaultGui
        } ~
          jocCompatibleRoute
      }
    }

  private def redirectToDefaultGui: Route = {
    htmlPreferred(webServiceContext) {
      val guiPath: Option[String] =
        if (schedulerConfiguration.existingHtmlDirOption.isDefined)
          Some("/jobscheduler/joc/")
        else
          webServiceContext.htmlEnabled option "/jobscheduler/master"
      passSome(guiPath) { path ⇒
        redirect(path, TemporaryRedirect)
      }
    }
  }

  private def masterRoute: Route =
    pathEndOrSingleSlash {
      htmlPreferred(webServiceContext) {
        redirect("/jobscheduler/master/api", TemporaryRedirect)
      }
    } ~
    pathPrefix("api") {
      apiRoute
    } ~
    (passIf(configuration.testMode) & pathPrefix("TEST")) {
      testRoute
    } ~
    requestInstance { request ⇒
      logger.debug(s"Rejected ${request.method} ${request.uri} ${request.headers}")
      reject
    }
}

object AllRoutes {
  private val logger = Logger(getClass)
}
