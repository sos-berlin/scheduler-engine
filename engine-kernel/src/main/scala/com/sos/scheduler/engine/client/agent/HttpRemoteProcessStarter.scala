package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClientFactory
import com.sos.scheduler.engine.agent.data.web.AgentUris
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.http.client.heartbeat.{HeartbeatRequestor, HttpHeartbeatTiming}
import com.sos.scheduler.engine.tunnel.client.WebTunnelClient
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HttpRemoteProcessStarter @Inject private(
  agentClientFactory: AgentClientFactory,
  newHeartbeatRequestor: HeartbeatRequestor.Factory,
  actorSystem: ActorSystem)
extends HasCloser {

  import actorSystem.dispatcher

  def startRemoteTask(
    schedulerApiTcpPort: Int,
    configuration: ApiProcessConfiguration,
    agentUri: String,
    httpHeartbeatTiming: Option[HttpHeartbeatTiming]): Future[HttpRemoteProcess] =
  {
    val port = schedulerApiTcpPort
    agentClientFactory.apply(agentUri).executeCommand(configuration.toUniversalAgentCommand) map { response ⇒
      new TunnelledHttpRemoteProcess {
        def actorSystem = HttpRemoteProcessStarter.this.actorSystem
        val agentClient = agentClientFactory.apply(agentUri)
        val processDescriptor = ProcessDescriptor.fromStartProcessResponse(response)
        def schedulerApiTcpPort = port
        val tunnelClient = new WebTunnelClient {
          def tunnelToken = processDescriptor.tunnelToken
          def tunnelUri = AgentUris(agentUri).tunnel(tunnelToken.id)
          val heartbeatRequestorOption = httpHeartbeatTiming map newHeartbeatRequestor
          def actorSystem = HttpRemoteProcessStarter.this.actorSystem
        }.closeWithCloser
      }
    }
  }
}
