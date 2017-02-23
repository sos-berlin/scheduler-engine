package com.sos.scheduler.engine.kernel.processclass.agent

import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.http.client.heartbeat.HttpHeartbeatTiming

/**
 * @author Joacim Zschimmer
 */
final case class Agent(
  /** To differentiate possible agents with same address. */
  id: Int,
  address: AgentAddress,
  httpHeartbeatTiming: Option[HttpHeartbeatTiming])
{
  require(address.nonEmpty, "An agent cannot be denoted by an empty address")
}
