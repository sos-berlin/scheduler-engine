package com.sos.scheduler.engine.plugins.newwebservice.routes.agent

import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait AgentRouteSchedulerAdapter {
  //this: AgentRoute ⇒

  protected def client: DirectSchedulerClient

  protected final def isKnownAgentUriFuture(uri: AgentAddress): Future[Boolean] =
    client.isKnownAgentUri(uri)
}
