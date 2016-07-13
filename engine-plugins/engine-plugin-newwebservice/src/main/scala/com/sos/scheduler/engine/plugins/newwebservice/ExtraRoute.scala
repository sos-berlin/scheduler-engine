package com.sos.scheduler.engine.plugins.newwebservice

import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait ExtraRoute {
  def route: Route
}
