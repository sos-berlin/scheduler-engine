package com.sos.scheduler.engine.agent.client

import spray.http.Uri

/**
 * URIs of the JobScheduler Agent.
 *
 * @author Joacim Zschimmer
 */
final class AgentUris private(agentUri: String) {

  val command = toAgentUri(Uri("command"))

  def fileStatus(filePath: String) = toAgentUri(Uri("fileStatus") withQuery ("file" → filePath))

  private def toAgentUri(u: Uri) = s"$agentUri/jobscheduler/agent/$u"
}

object AgentUris {
  def apply(agentUri: String) = new AgentUris(agentUri stripSuffix "/")
}
