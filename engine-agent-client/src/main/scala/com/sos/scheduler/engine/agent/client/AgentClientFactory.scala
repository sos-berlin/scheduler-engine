package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import javax.inject.{Inject, Singleton}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class AgentClientFactory @Inject private[client](implicit actorSystem: ActorSystem) {

  def apply(agentUri: String): AgentClient = {
    val pAgentUri = agentUri
    new AgentClient {
      protected val actorSystem = AgentClientFactory.this.actorSystem
      protected[client] val agentUri = pAgentUri
    }
  }
}
