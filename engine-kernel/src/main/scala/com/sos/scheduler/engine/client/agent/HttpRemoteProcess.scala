package com.sos.scheduler.engine.client.agent

import com.sos.jobscheduler.agent.client.AgentClient
import com.sos.jobscheduler.agent.data.commandresponses.EmptyResponse
import com.sos.jobscheduler.agent.data.commands.{CloseTask, SendProcessSignal}
import com.sos.jobscheduler.base.process.ProcessSignal
import scala.concurrent.{ExecutionContext, Future}

/**
 * A remote process started by [[HttpRemoteProcessStarter]].
 *
 * @author Joacim Zschimmer
 */
trait HttpRemoteProcess extends AutoCloseable {

  protected def agentClient: AgentClient
  protected def processDescriptor: ProcessDescriptor
  protected implicit def executionContext: ExecutionContext

  def start(): Unit

  final def sendSignal(processSignal: ProcessSignal): Future[Unit] =
    agentClient.executeCommand(SendProcessSignal(processDescriptor.agentTaskId, processSignal)) map { _: EmptyResponse.type ⇒ () }

  final def closeRemoteTask(kill: Boolean): Future[Unit] =
    agentClient.executeCommand(CloseTask(processDescriptor.agentTaskId, kill = kill)) map { _: EmptyResponse.type ⇒ () }
}
