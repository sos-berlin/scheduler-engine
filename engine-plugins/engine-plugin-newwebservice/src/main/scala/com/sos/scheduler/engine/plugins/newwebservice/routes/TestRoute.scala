package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.plugins.newwebservice.common.MyDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait TestRoute {
  protected def configuration: NewWebServicePluginConfiguration

  protected def testRoute: Route =
    if (configuration.testMode)
      get {
        path("OutOfMemoryError") {
          toughComplete {
            throw new OutOfMemoryError
          }
        } ~
        path("ERROR-500") {
          complete {
            sys.error("ERROR " * 10)
          }
        }
      }
    else
      reject
}
