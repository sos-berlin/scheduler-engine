package com.sos.scheduler.engine.kernel.processclass.agent

/**
 * @author Joacim Zschimmer
 */
final case class Agent(
  /** To differentiate possible agents with same address. */
  id: Int,
  address: String)
{
  require(address.nonEmpty, "An agent cannot be denoted by an empty address")
}
