package com.sos.scheduler.agent.configuration

/**
 * @author Joacim Zschimmer
 */
final case class AgentConfiguration(
  httpPort: Int,
  httpInterfaceRestriction: Option[String] = None)
