package com.sos.scheduler.engine.agent.data.commands

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
