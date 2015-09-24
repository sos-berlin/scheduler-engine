package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClientFactory
import com.sos.scheduler.engine.agent.data.web.AgentUris
import com.sos.scheduler.engine.tunnel.client.WebTunnelClient
import com.sos.scheduler.engine.tunnel.data.TunnelId
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HttpRemoteProcessStarter @Inject private(
  agentClientFactory: AgentClientFactory,
  actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  def startRemoteTask(schedulerApiTcpPort: Int, configuration: ApiProcessConfiguration, agentUri: String): Future[HttpRemoteProcess] =
    agentClientFactory.apply(agentUri).executeCommand(configuration.toUniversalAgentCommand) map { response ⇒
      new TunnelledHttpRemoteProcess(
        actorSystem,
        agentClient = agentClientFactory.apply(agentUri),
        processDescriptor = ProcessDescriptor.fromStartProcessResponse(response),
        schedulerApiTcpPort = schedulerApiTcpPort,
        tunnelClient = new WebTunnelClient {
          def tunnelUri(id: TunnelId) = AgentUris(agentUri).tunnel(id)
          def actorSystem = HttpRemoteProcessStarter.this.actorSystem
        })
  }
}
