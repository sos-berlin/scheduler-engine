package com.sos.scheduler.engine.agent.xmlcommand

import com.sos.scheduler.engine.agent.commands.{CloseRemoteTaskResponse, Command, Response, StartRemoteTaskResponse}
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future

/**
 * Executes public Agent command serialized as XML string.
 * @author Joacim Zschimmer
 */
final class CommandXmlExecutor(executeCommand: Command ⇒ Future[Response]) {

  def execute(commandString: String): Future[xml.Elem] =
    (Future { CommandXml.parseString(commandString) }
      flatMap executeCommand
      map responseToXml
      recover { case throwable ⇒ <ERROR text={throwable.toString}/> }
      map { contentElem ⇒ <spooler><answer>{contentElem}</answer></spooler> })

  private def responseToXml(response: Response): xml.Elem = response match {
    case o: StartRemoteTaskResponse ⇒ StartRemoteTaskXml.responseToXmlElem(o)
    case CloseRemoteTaskResponse ⇒ CloseRemoteTaskXml.responseXmlElem
  }
}
