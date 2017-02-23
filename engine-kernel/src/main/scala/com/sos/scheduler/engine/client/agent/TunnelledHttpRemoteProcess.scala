package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.jobscheduler.agent.client.AgentClient
import com.sos.jobscheduler.common.scalautil.Closers.implicits._
import com.sos.jobscheduler.common.scalautil.HasCloser
import com.sos.jobscheduler.tunnel.client.{TcpToHttpBridge, WebTunnelClient}
import java.net.InetSocketAddress

/**
 * A remote process started by [[HttpRemoteProcessStarter]], with API calls tunnelled via HTTP and Agent.
 *
 * @author Joacim Zschimmer
 */
private[agent] trait TunnelledHttpRemoteProcess extends HasCloser with HttpRemoteProcess {

  protected def actorSystem: ActorSystem
  protected def agentClient: AgentClient
  protected def processDescriptor: ProcessDescriptor
  protected def schedulerApiTcpPort: Int
  protected val tunnelClient: WebTunnelClient

  protected def executionContext = actorSystem.dispatcher

  private lazy val tcpHttpBridge = new TcpToHttpBridge(
    actorSystem,
    connectTo = new InetSocketAddress("127.0.0.1", schedulerApiTcpPort),
    tunnelToken = processDescriptor.tunnelToken,
    tunnelClient = tunnelClient)
  .closeWithCloser

  def start() = tcpHttpBridge.start()
}
