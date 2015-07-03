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
    val universalClient = agentClientFactory.apply(agentUri)
    agentUri match {
      case HasClassicAgentPrefix(classicAgentUri) ⇒
        val classicClient = schedulerClientFactory.apply(classicAgentUri)
        (classicClient.uncheckedExecute(configuration.toClassicXmlElem(schedulerApiTcpPort))
          map ProcessDescriptor.fromXml
          map { pd ⇒ new HttpRemoteProcess.Standard(classicClient, pd, actorSystem.dispatcher) })
      case _ ⇒
        val classicClient = schedulerClientFactory.apply(agentUri)
        universalClient.executeCommand(configuration.toUniversalAgentCommand) map { response ⇒
          val processDescriptor = ProcessDescriptor.fromStartProcessResponse(response)
          val tunnelToken = response.tunnelTokenOption.getOrElse { sys.error(s"Missing TunnelToken from agent $agentUri") }
          val tunnelClient = new WebTunnelClient {
            protected def tunnelUri(id: TunnelId) = AgentUris(agentUri).tunnelItem(id)
            protected def actorRefFactory = actorSystem
          }
          new TunnelledHttpRemoteProcess(actorSystem, classicClient, processDescriptor, schedulerApiTcpPort = schedulerApiTcpPort, tunnelClient, tunnelToken)
        }
    }
  }
}

object HttpRemoteProcessStarter {
  private val HasClassicAgentPrefix = "classic:(.*)".r
}
