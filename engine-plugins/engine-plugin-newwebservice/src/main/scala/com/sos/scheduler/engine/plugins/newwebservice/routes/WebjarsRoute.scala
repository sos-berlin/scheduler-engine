package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.plugins.newwebservice.routes.WebjarsRoute._
import scala.util.control.NonFatal
import spray.http.CacheDirectives.`max-age`
import spray.http.HttpHeaders.`Cache-Control`
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait WebjarsRoute {

  protected implicit def actorRefFactory: ActorRefFactory
  protected lazy val webjarsExists = {
    var result = true
    for (resource ← Needed map { o ⇒ WebjarsResourceDirectory / o })
      try resource.url
      catch { case NonFatal(t) ⇒
        logger.debug(s"Missing JavaResource $resource: $t")
        result = false
      }
    result
  }

  protected final def webjarsRoute: Route =
    get {
      respondWithHeader(LongTimeCaching) {
        getFromResourceDirectory(WebjarsResourceDirectory.path)    // Artifacts of Maven groupId 'org.webjars'
      }
    }
}

object WebjarsRoute {
  val BootstrapCss = "bootstrap/3.3.6/css/bootstrap.min.css"
  private val Needed = List(BootstrapCss)
  private val WebjarsResourceDirectory = JavaResource("META-INF/resources/webjars")

  private val LongTimeCaching = `Cache-Control`(`max-age`((30 * 24.h).getSeconds))
  private val logger = Logger(getClass)
}
