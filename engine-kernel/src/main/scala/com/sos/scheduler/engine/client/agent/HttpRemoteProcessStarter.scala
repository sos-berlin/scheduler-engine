package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.{AgentClientFactory, AgentUris}
import com.sos.scheduler.engine.client.agent.HttpRemoteProcessStarter._
import com.sos.scheduler.engine.client.command.SchedulerClientFactory
import com.sos.scheduler.engine.tunnel.client.WebTunnelClient
import com.sos.scheduler.engine.tunnel.data.TunnelId
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HttpRemoteProcessStarter @Inject private(
  schedulerClientFactory: SchedulerClientFactory,
  agentClientFactory: AgentClientFactory,
  actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  def startRemoteTask(schedulerApiTcpPort: Int, configuration: ApiProcessConfiguration, agentUri: String): Future[HttpRemoteProcess] = {
    agentUri match {
      case HasClassicAgentPrefix(classicAgentUri) ⇒
        val classicClient = schedulerClientFactory.apply(classicAgentUri)
        (classicClient.uncheckedExecute(configuration.toClassicXmlElem(schedulerApiTcpPort))
          map ProcessDescriptor.fromXml
          map { pd ⇒ new HttpRemoteProcess.Standard(classicClient, pd, actorSystem.dispatcher) })
      case _ ⇒
        agentClientFactory.apply(agentUri).executeCommand(configuration.toUniversalAgentCommand) map { response ⇒
          new TunnelledHttpRemoteProcess(
            actorSystem,
            classicClient = schedulerClientFactory.apply(agentUri),
            processDescriptor = ProcessDescriptor.fromStartProcessResponse(response),
            schedulerApiTcpPort = schedulerApiTcpPort,
            tunnelClient = new WebTunnelClient {
              def tunnelUri(id: TunnelId) = AgentUris(agentUri).tunnelHandler.item(id)
              def actorSystem = HttpRemoteProcessStarter.this.actorSystem
            })
        }
    }
  }
}

object HttpRemoteProcessStarter {
  private val HasClassicAgentPrefix = "classic:(.*)".r
}
