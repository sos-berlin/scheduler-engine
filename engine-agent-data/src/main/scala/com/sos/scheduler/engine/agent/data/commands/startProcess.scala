package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.responses.StartProcessResponse

/**
 * @author Joacim Zschimmer
 */
sealed trait StartProcess extends ProcessCommand {
  type Response = StartProcessResponse
  val controllerAddress: String
}

object StartProcess {
  val XmlElementName = "remote_scheduler.start_remote_task"
}

final case class StartThread(controllerAddress: String)
extends StartProcess

final case class StartSeparateProcess(controllerAddress: String, javaOptions: String, javaClasspath: String)
extends StartProcess
