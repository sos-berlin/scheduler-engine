package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.tunnel.AgentTunnelClient
import com.sos.scheduler.engine.client.command.SchedulerCommandClient
import com.sos.scheduler.engine.tunnel.TcpToRequestResponse
import com.sos.scheduler.engine.tunnel.data.TunnelToken
import java.net.InetSocketAddress

/**
 * A remote process started by [[HttpRemoteProcessStarter]], with API calls tunnelled via HTTP and Agent.
 *
 * @author Joacim Zschimmer
 */
private[agent] final class TunnelledHttpRemoteProcess(
  actorSystem: ActorSystem,
  protected val classicClient: SchedulerCommandClient,
  protected val processDescriptor: ProcessDescriptor,
  schedulerApiTcpPort: Int,
  tunnelClient: AgentTunnelClient,
  tunnelToken: TunnelToken)
extends HttpRemoteProcess {

  protected def executionContext = actorSystem.dispatcher

  private val tcpHttpBridge = new TcpToRequestResponse(
    actorSystem,
    remoteAddress = new InetSocketAddress("127.0.0.1", schedulerApiTcpPort),
    executeRequest = request ⇒ tunnelClient.tunnelRequest(tunnelToken, request))

  def start() = tcpHttpBridge.start()

  def close() = tcpHttpBridge.close()
}
