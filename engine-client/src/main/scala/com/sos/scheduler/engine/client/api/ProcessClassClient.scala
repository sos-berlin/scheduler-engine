package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.event.Snapshot
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait ProcessClassClient {
  def agentUris: Future[Snapshot[Set[AgentAddress]]]
}
