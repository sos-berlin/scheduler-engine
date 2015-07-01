package com.sos.scheduler.engine.agent.client.tunnel

import akka.actor.ActorSystem
import akka.util.ByteString
import com.sos.scheduler.engine.agent.client.AgentUris
import com.sos.scheduler.engine.common.sprayutils.ByteStreamMarshallers._
import com.sos.scheduler.engine.tunnel.data.Http._
import com.sos.scheduler.engine.tunnel.data.TunnelId
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.HttpHeaders.Accept
import spray.http.HttpRequest
import spray.http.MediaTypes._
import spray.httpx.encoding.Gzip

/**
 * @author Joacim Zschimmer
 */
trait AgentTunnelClient {
  import actorSystem.dispatcher

  protected def agentUris: AgentUris
  implicit protected val actorSystem: ActorSystem

  private lazy val pipelineTrunk: HttpRequest ⇒ Future[ByteString] =
    addHeader(Accept(`application/octet-stream`)) ~>
      encode(Gzip) ~>
      sendReceive ~>
      decode(Gzip) ~>
      unmarshal[ByteString]

  private[tunnel] final def tunnelRequest(tunnelIdWithPassword: TunnelId.WithPassword, requestMessage: ByteString): Future[ByteString] = {
    val pipeline = addHeader(PasswordHeaderName, tunnelIdWithPassword.password.string) ~> pipelineTrunk
    pipeline(Post(agentUris.tunnel(tunnelIdWithPassword.id), requestMessage))
  }

  def tunnelClient(tunnelIdWithPassword: TunnelId.WithPassword) = new TunnelClient(this, tunnelIdWithPassword)
}
