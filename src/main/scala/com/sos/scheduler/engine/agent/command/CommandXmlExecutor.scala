package com.sos.scheduler.engine.agent.command

import com.sos.scheduler.engine.agent.commands.{Command, Response}
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
final class CommandXmlExecutor(executeCommand: Command ⇒ Future[Response]) {

  def execute(commandString: String): Future[xml.Elem] =
    (Future { Command.parseString(commandString) }
      flatMap executeCommand
      map { response ⇒ <spooler><answer>{response.toElem}</answer></spooler> }
      recover { case throwable ⇒ <spooler><answer><ERROR text={throwable.toString}/></answer></spooler> })
}
