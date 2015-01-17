package com.sos.scheduler.engine.agent.commands

import com.sos.scheduler.engine.data.agent.RemoteTaskId

/**
 * @author Joacim Zschimmer
 */
sealed trait Command


/**
 * @author Joacim Zschimmer
 */
sealed trait Response


/**
 * @author Joacim Zschimmer
 */
trait RemoteTaskCommand extends Command


final case class StartRemoteTask(
  controllerTcpPort: Int,
  usesApi: Boolean,
  javaOptions: String = "",
  javaClassPath: String = "")
extends RemoteTaskCommand


/**
 * @author Joacim Zschimmer
 */
final case class StartRemoteTaskResponse(remoteTaskId: RemoteTaskId) extends Response


/**
 * @author Joacim Zschimmer
 */
final case class CloseRemoteTask(remoteTaskId: RemoteTaskId, kill: Boolean)
extends RemoteTaskCommand


/**
 * @author Joacim Zschimmer
 */
object CloseRemoteTaskResponse extends Response
