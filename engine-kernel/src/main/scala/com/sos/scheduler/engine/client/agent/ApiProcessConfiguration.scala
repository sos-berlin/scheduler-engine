package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.data.commands.{StartSeparateProcess, StartThread}

/**
 * @author Joacim Zschimmer
 */
final case class ApiProcessConfiguration(
  hasApi: Boolean,
  javaOptions: String,
  javaClasspath: String) {

  def toUniversalAgentCommand =
    if (hasApi) StartSeparateProcess(javaOptions = javaOptions, javaClasspath = javaClasspath)
    else StartThread
}
