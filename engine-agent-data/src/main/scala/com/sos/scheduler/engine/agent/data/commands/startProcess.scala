package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId

/**
 * @author Joacim Zschimmer
 */
trait StartProcess extends ProcessCommand {
  type Response = StartProcessResponse
  val controllerAddress: String
}

final case class StartThread(controllerAddress: String)
extends StartProcess

final case class StartSeparateProcess(controllerAddress: String, javaOptions: String, javaClasspath: String)
extends StartProcess


/**
 * @author Joacim Zschimmer
 */
final case class StartProcessResponse(processId: AgentProcessId) extends Response

