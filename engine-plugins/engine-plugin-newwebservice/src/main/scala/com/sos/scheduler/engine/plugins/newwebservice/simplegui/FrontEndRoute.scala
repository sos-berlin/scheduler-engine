package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.FrontEndRoute._
import spray.http.CacheDirectives.`min-fresh`
import spray.http.HttpHeaders.`Cache-Control`
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait FrontEndRoute extends WebjarsRoute {

  final def frontEndRoute: Route =
    pathPrefix("webjars") {
      webjarsRoute
    } ~
    get {
      respondWithHeader(`Cache-Control`(`min-fresh`(60))) {
        getFromResourceDirectory(FrontendResourceDirectory.path)
      }
    }
}

object FrontEndRoute {
  private val FrontendResourceDirectory = JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend")
}
