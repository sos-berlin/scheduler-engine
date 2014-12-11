package com.sos.scheduler.agent

import com.sos.scheduler.agent.CommandExecutor._
import com.sos.scheduler.agent.command._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.taskserver.TaskServer
import com.sos.scheduler.taskserver.configuration.StartConfiguration
import java.net.InetSocketAddress
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent._


/**
 * @author Joacim Zschimmer
 */
final class CommandExecutor extends (Command ⇒ Response) {

  def apply(command: Command): Response = {
    command match {
      case StartRemoteTask(controllerTcpPort, usesApi, javaOptions, javaClassPath) ⇒
        Future {  // ???
          blocking {
            try TaskServer.run(StartConfiguration(controllerAddress = new InetSocketAddress("127.0.0.1", controllerTcpPort)))
            catch {
              case t: Throwable ⇒
                logger.error(t.toString, t)
                throw t
            }
          }
        }
        RemoteTaskStartedResponse(StandardProcessId)

      case CloseRemoteTask(StandardProcessId, kill) ⇒
        // TaskServer beenden ???
        CloseRemoteTaskResponse
    }
  }

}

private object CommandExecutor {
  private val logger = Logger(getClass)

  private val StandardProcessId = 4711  // ???
}
