package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.responses.StartProcessResponse

/**
 * @author Joacim Zschimmer
 */
trait StartProcess extends ProcessCommand {
  type Response = StartProcessResponse
  val controllerAddressOption: Option[String]
}

object StartProcess {
  val XmlElementName = "remote_scheduler.start_remote_task"
}
