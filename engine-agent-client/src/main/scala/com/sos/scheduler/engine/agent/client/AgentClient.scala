package com.sos.scheduler.engine.agent.client

import com.sos.scheduler.engine.agent.data.commands._
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
trait AgentClient {

  def executeCommand(command: Command): Future[command.Response]

  def fileExists(file: String): Future[Boolean]
}
