package com.sos.scheduler.engine.agent.client.tunnel

import akka.util.ByteString
import com.sos.scheduler.engine.tunnel.data.TunnelToken
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
final class TunnelClient private[tunnel](agentClient: AgentTunnelClient, tunnelToken: TunnelToken) {

  def tunnelRequest(requestMessage: ByteString): Future[ByteString] = agentClient.tunnelRequest(tunnelToken, requestMessage)
}
