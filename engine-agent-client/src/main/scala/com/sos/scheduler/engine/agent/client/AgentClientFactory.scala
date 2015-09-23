package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import javax.inject.{Inject, Singleton}
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class AgentClientFactory @Inject private[client](implicit actorSystem: ActorSystem, licenseKeys: immutable.Iterable[LicenseKeyString]) {

  def apply(agentUri: String): AgentClient = {
    val pAgentUri = agentUri
    new AgentClient {
      val agentUri = pAgentUri
      def licenseKeys = AgentClientFactory.this.licenseKeys
      val actorRefFactory = AgentClientFactory.this.actorSystem
    }
  }
}
