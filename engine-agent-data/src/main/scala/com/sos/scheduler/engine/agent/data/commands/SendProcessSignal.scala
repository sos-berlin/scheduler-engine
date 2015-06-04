package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId
import com.sos.scheduler.engine.agent.data.responses.EmptyResponse
import com.sos.scheduler.engine.base.process.ProcessSignal
import spray.json.DefaultJsonProtocol._

/**
 * Sends a Unix type process signal like SIGTERM to a process.
 *
 * @author Joacim Zschimmer
 */
final case class SendProcessSignal(processId: AgentProcessId, signal: ProcessSignal)
extends ProcessCommand {
  type Response = EmptyResponse.type
}

object SendProcessSignal {
  val XmlElementName = "remote_scheduler.remote_task.kill"
  private implicit def UnixProcessSignalJsonFormat = ProcessSignal.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat2(apply)
}
