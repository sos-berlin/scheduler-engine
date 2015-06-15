package com.sos.scheduler.engine.agent.main

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing

/**
 * JobScheduler Agent.
 *
 * @author Joacim Zschimmer
 */
object AgentMain {
  def main(args: Array[String]): Unit = runWithGui(AgentConfiguration(args))

  def runWithGui(conf: AgentConfiguration): Unit =
    autoClosing(new Agent(conf)) { _.run() }
}
