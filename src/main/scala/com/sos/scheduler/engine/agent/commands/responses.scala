package com.sos.scheduler.engine.agent.commands

import com.sos.scheduler.engine.agent.task.RemoteTaskId

/**
 * @author Joacim Zschimmer
 */
sealed trait Response {
  def toElem: xml.Elem
}

/**
 * @author Joacim Zschimmer
 */
final case class StartRemoteTaskResponse(remoteTaskId: RemoteTaskId) extends Response {
  def toElem = <process process_id={remoteTaskId.value.toString}/>
}

/**
 * @author Joacim Zschimmer
 */
object CloseRemoteTaskResponse extends Response {
  def toElem = <ok/>
}
