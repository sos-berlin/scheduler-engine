package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.plugins.newwebservice.routes.WebjarsRoute._
import spray.http.CacheDirectives.`max-age`
import spray.http.HttpHeaders.`Cache-Control`
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait WebjarsRoute {

  protected implicit def actorRefFactory: ActorRefFactory

  protected final def webjarsRoute: Route =
    get {
      respondWithHeader(LongTimeCaching) {
        getFromResourceDirectory("META-INF/resources/webjars")    // Artifacts of Maven groupId 'org.webjars'
      }
    }
}

object WebjarsRoute {
  private val LongTimeCaching = `Cache-Control`(`max-age`((30 * 24.h).getSeconds))
}
