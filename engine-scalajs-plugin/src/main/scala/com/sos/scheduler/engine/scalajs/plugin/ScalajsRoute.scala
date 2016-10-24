package com.sos.scheduler.engine.scalajs.plugin

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.sprayutils.ContentTypedString.HtmlString
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.plugins.newwebservice.ExtraRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.WebjarsRoute
import javax.inject.{Inject, Singleton}
import spray.http.CacheDirectives.{`max-age`, `no-cache`, `no-store`}
import spray.http.HttpHeaders.`Cache-Control`
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
@Singleton
private[scalajs] final class ScalajsRoute @Inject private(implicit protected val actorRefFactory: ActorRefFactory)
extends ExtraRoute with WebjarsRoute {

  private val ResourceDirectory = JavaResource("com/scheduler/engine/scalajs/frontend")

  def route: Route =
    (decompressRequest() & compressResponseIfRequested(())) {
      pathPrefix("jobscheduler" / "master" / "scalajs") {
//        (pathEnd & get) {
//          redirect("scalajs/", TemporaryRedirect)
//        } ~
        respondWithHeader(`Cache-Control`(`max-age`(0), `no-store`, `no-cache`)) {
          pathEnd {
          //pathSingleSlash {
            complete(HtmlString(new SingleScalaJsPage().wholePage.render))
          } ~
          dynamic {
            getFromResourceDirectory(ResourceDirectory.path)
          }
        }
      }
    }
}
