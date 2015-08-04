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
    if (hasApi) StartSeparateProcess(controllerAddressOption = None, javaOptions = javaOptions, javaClasspath = javaClasspath)
    else StartThread(controllerAddressOption = None)
}
