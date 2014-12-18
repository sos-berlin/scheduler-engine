package com.sos.scheduler.engine.agent.configuration

/**
 * @author Joacim Zschimmer
 */
final case class AgentConfiguration(
  httpPort: Int,
  httpInterfaceRestriction: Option[String] = None)
