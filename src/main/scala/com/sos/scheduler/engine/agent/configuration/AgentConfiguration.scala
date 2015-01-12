package com.sos.scheduler.engine.agent.configuration

import com.sos.scheduler.engine.agent.common.CommandLineArguments

/**
 * @author Joacim Zschimmer
 */
final case class AgentConfiguration(
  httpPort: Int,
  httpInterfaceRestriction: Option[String] = None)

object AgentConfiguration {
  def apply(args: Seq[String]) = {
    val arguments = CommandLineArguments(args)
    val httpPort = arguments.int("-http-port=")
    arguments.requireNoMoreArguments()
    new AgentConfiguration(httpPort = httpPort)
  }
}
