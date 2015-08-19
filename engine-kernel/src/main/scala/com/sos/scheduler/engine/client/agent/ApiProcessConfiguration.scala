package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.data.commands.{StartApiTask, StartNonApiTask}

/**
 * @author Joacim Zschimmer
 */
final case class ApiProcessConfiguration(
  hasApi: Boolean,
  javaOptions: String,
  javaClasspath: String) {

  def toUniversalAgentCommand =
    if (hasApi) StartApiTask(javaOptions = javaOptions, javaClasspath = javaClasspath)
    else StartNonApiTask
}
