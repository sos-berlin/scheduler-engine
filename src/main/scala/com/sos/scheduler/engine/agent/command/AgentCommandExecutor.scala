package com.sos.scheduler.engine.agent.command

import com.sos.scheduler.engine.agent.commands.{Command, RemoteTaskCommand, Response}
import com.sos.scheduler.engine.agent.task.RemoteTaskHandler
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class AgentCommandExecutor @Inject private(remoteTaskHandler: RemoteTaskHandler)
extends CommandExecutor {

  //TODO Threadsicher? Als Actor implementieren?
  def executeCommand(command: Command): Future[Response] =
    command match {
      case command: RemoteTaskCommand ⇒ remoteTaskHandler.executeCommand(command)
      //case Terminate ⇒ ???
    }
}
