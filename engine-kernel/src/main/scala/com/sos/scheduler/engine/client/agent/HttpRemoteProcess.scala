package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.client.AgentClient
import com.sos.scheduler.engine.agent.data.commands.{CloseProcess, SendProcessSignal}
import com.sos.scheduler.engine.agent.data.responses.EmptyResponse
import com.sos.scheduler.engine.base.process.ProcessSignal
import scala.concurrent.{ExecutionContext, Future}

/**
 * A remote process started by [[HttpRemoteProcessStarter]].
 *
 * @author Joacim Zschimmer
 */
trait HttpRemoteProcess {

  protected def agentClient: AgentClient
  protected def processDescriptor: ProcessDescriptor
  protected implicit def executionContext: ExecutionContext

  def start(): Unit
  def close(): Unit

  final def sendSignal(processSignal: ProcessSignal): Future[Unit] =
    agentClient.executeCommand(SendProcessSignal(processDescriptor.agentProcessId, processSignal)) map { _: EmptyResponse.type ⇒ () }

  final def closeRemoteTask(kill: Boolean): Future[Unit] =
    agentClient.executeCommand(CloseProcess(processDescriptor.agentProcessId, kill = kill)) map { _: EmptyResponse.type ⇒ () }

  override def toString = s"${getClass.getSimpleName}(${processDescriptor.agentProcessId} pid=${processDescriptor.pid})"
}
