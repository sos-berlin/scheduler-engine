package com.sos.scheduler.agent.command

import javax.inject.{Inject, Singleton}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class StringCommandExecutor @Inject private(executeCommand: Command ⇒ Response)
extends (String ⇒ String) {

  def apply(commandString: String): String = {
    val command = CommandXmlParser.parseCommand(commandString)
    val response = executeCommand.apply(command)
    <spooler><answer>{response.toElem}</answer></spooler>.toString()
  }
}
