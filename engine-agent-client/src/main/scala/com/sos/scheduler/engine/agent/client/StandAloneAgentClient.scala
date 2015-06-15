package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem

/**
 * Stand-alone client for JobScheduler Agent.
 *
 * @author Joacim Zschimmer
 */
final class StandAloneAgentClient private(protected[client] val agentUri: String) extends AgentClient with AutoCloseable {

  protected val actorSystem = ActorSystem("SimpleAgentClient")

  def close() = actorSystem.shutdown()
}

object StandAloneAgentClient {
  def apply(agentUri: String) = new StandAloneAgentClient(agentUri)
}
