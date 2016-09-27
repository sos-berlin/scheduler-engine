package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import javax.inject.{Inject, Singleton}
import scala.collection.immutable
import spray.http.Uri

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class AgentClientFactory @Inject private[client](implicit actorSystem: ActorSystem, licenseKeys: immutable.Iterable[LicenseKeyString]) {

  def apply(agentUri: String): AgentClient = {
    val agentUri_ = Uri(agentUri)
    new AgentClient {
      val agentUri = agentUri_
      def licenseKeys = AgentClientFactory.this.licenseKeys
      val actorRefFactory = AgentClientFactory.this.actorSystem
    }
  }
}
