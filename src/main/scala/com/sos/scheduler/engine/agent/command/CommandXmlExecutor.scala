package com.sos.scheduler.engine.agent.command

import com.sos.scheduler.engine.agent.commands.Command
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class CommandXmlExecutor @Inject private(executeCommand: CommandExecutor) {

  def execute(commandString: String): Future[xml.Elem] = {
    val command = Command.parseString(commandString)
    executeCommand.executeCommand(command) map { response ⇒
      <spooler><answer>{response.toElem}</answer></spooler>
    }
  }
}
