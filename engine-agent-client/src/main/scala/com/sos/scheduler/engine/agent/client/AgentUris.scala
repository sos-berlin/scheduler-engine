package com.sos.scheduler.engine.agent.client

import com.sos.scheduler.engine.tunnel.data.TunnelId
import spray.http.Uri
import spray.http.Uri.Path

/**
 * URIs of the JobScheduler Agent.
 *
 * @author Joacim Zschimmer
 */
final class AgentUris private(agentUri: String) {

  val command: String =
    uriString(Path("command"))

  def tunnel(tunnelId: TunnelId): String =
    uriString(Path("tunnel") / tunnelId.string)

  def overview: String =
    uriString(Path("overview"))

  def fileStatus(filePath: String): String =
    (toAgentUri(Path("fileStatus")) withQuery ("file" → filePath)).toString()

  def apply(relativeUri: String): String =
    uriString(Path(relativeUri stripPrefix "/"))

  private def uriString(path: Path) = toAgentUri(path).toString

  private def toAgentUri(path: Path) = Uri(agentUri) withPath Path(s"/jobscheduler/agent/$path")

  override def toString = agentUri
}

object AgentUris {
  def apply(agentUri: String) = new AgentUris(agentUri stripSuffix "/")
}
