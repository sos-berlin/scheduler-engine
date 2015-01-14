package com.sos.scheduler.engine.agent.web

import spray.routing.HttpServiceActor

/**
 * @author Joacim Zschimmer
 */
trait AgentWebServiceActor extends HttpServiceActor with AgentRoute {
  def receive = runRoute(route)
}
