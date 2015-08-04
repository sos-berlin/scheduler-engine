package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.agent.data.responses.EmptyResponse
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class CloseProcess(processId: AgentProcessId, kill: Boolean)
extends ProcessCommand {
  type Response = EmptyResponse.type
}

object CloseProcess {
  val SerialTypeName = "CloseProcess"
  implicit val MyJsonFormat = jsonFormat2(apply)
}
