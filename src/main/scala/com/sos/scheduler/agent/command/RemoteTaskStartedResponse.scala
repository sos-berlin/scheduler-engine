package com.sos.scheduler.agent.command

import com.sos.scheduler.agent.command.RemoteTaskStartedResponse._

/**
 * @author Joacim Zschimmer
 */
final case class RemoteTaskStartedResponse(processId: Long) extends Response {
  def toElem = <process process_id={StandardProcessId.toString}/>
}

object RemoteTaskStartedResponse {
  val StandardProcessId = 4711 //???
}
