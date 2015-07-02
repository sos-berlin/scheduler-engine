package com.sos.scheduler.engine.agent.client

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

  def overview: String =
    uriString(Path("overview"))

  def fileStatus(filePath: String): String =
    (withPath(Path("fileStatus")) withQuery ("file" → filePath)).toString()

  def apply(relativeUri: String): String =
    uriString(Path(relativeUri stripPrefix "/"))

  private def uriString(path: Path) = withPath(path).toString

  def withPath(path: Path) = Uri(agentUri) withPath Path(s"/jobscheduler/agent/${path.toString stripPrefix "/"}")

  override def toString = agentUri
}

object AgentUris {
  def apply(agentUri: String) = new AgentUris(agentUri stripSuffix "/")
}
