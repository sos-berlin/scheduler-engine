package com.sos.scheduler.engine.agent.command

import com.sos.scheduler.engine.agent.commands.{Command, StartRemoteTask, StartRemoteTaskResponse}
import com.sos.scheduler.engine.data.agent.RemoteTaskId
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.duration.DurationDouble
import scala.concurrent.{Await, Future}

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class CommandXmlExecutorTest extends FreeSpec {

  "CommandXmlExecutor" in {
    def executeCommand(command: Command) = command match {
      case StartRemoteTask(999, true, "", "") ⇒ Future { throw new Exception }
      case StartRemoteTask(1000, true, "", "") ⇒ Future { StartRemoteTaskResponse(RemoteTaskId(999)) }
      case o ⇒ fail(o.toString)
    }
    def execute(command: String): Unit = {
      val executed = new CommandXmlExecutor(executeCommand).execute(command)
      Await.result(executed, 1.seconds) match {
        case <spooler><answer>{elem: xml.Elem}</answer></spooler> ⇒ if (elem.label == "ERROR") throw new CommandException
      }
    }
    intercept[CommandException] { execute("INVALID XML") }
    intercept[CommandException] { execute("<WRONG/>") }
    intercept[CommandException] { execute("<remote_scheduler.start_remote_task tcp_port='WRONG'/>") }
    intercept[CommandException] { execute("<remote_scheduler.start_remote_task tcp_port='999'/>") }
    execute("<remote_scheduler.start_remote_task tcp_port='1000'/>")
  }

  private class CommandException extends Exception
}
