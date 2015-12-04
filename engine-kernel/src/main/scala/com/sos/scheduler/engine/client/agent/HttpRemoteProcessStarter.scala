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
    val timing = httpHeartbeatTiming
    agentClientFactory.apply(agentUri).executeCommand(configuration.toUniversalAgentCommand) map { response ⇒
      val processDescriptor_ = ProcessDescriptor.fromStartProcessResponse(response)
      val tunnelClient_ = new WebTunnelClient {
        def tunnelToken = processDescriptor_.tunnelToken
        def tunnelUri = AgentUris(agentUri).tunnel(tunnelToken.id)
        def heartbeatTimingOption = timing
        val heartbeatRequestorOption = timing map newHeartbeatRequestor
        def actorSystem = HttpRemoteProcessStarter.this.actorSystem
      }.closeWithCloser
      new TunnelledHttpRemoteProcess {
        def actorSystem = HttpRemoteProcessStarter.this.actorSystem
        val agentClient = agentClientFactory.apply(agentUri)
        val processDescriptor = processDescriptor_
        def schedulerApiTcpPort = port
        val tunnelClient = tunnelClient_
      }
    }
  }
}
