package com.sos.scheduler.engine.kernel.processclass.agent

import akka.pattern.AskTimeoutException
import com.sos.scheduler.engine.base.process.ProcessSignal.SIGTERM
import com.sos.scheduler.engine.client.agent.{ApiProcessConfiguration, HttpRemoteProcess, HttpRemoteProcessStarter}
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.http.client.heartbeat.HttpHeartbeatTiming
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.processclass.agent.CppHttpRemoteApiProcessClient._
import com.sos.scheduler.engine.kernel.processclass.common.{FailableCollection, FailableSelector}
import java.time.Duration
import javax.inject.{Singleton, Inject, Provider}
import scala.concurrent.Future
import scala.util.{Failure, Success, Try}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class CppHttpRemoteApiProcessClient private(
  apiProcessConfiguration: ApiProcessConfiguration, schedulerApiTcpPort: Int, warningCall: CppCall, resultCall: CppCall,
  starter: HttpRemoteProcessStarter, callQueue: SchedulerThreadCallQueue)
extends AutoCloseable {

  import callQueue.implicits.executionContext

  private object callbacks extends FailableSelector.Callbacks[Agent, HttpRemoteProcess] {
    def apply(agent: Agent): Future[Try[HttpRemoteProcess]] = {
      val future = starter.startRemoteTask(
        schedulerApiTcpPort,
        apiProcessConfiguration,
        agentUri = agent.address,
        Some(agent.httpHeartbeatTiming getOrElse HttpHeartbeatTiming.Default))
      .withThisStackTrace
      future map Success.apply recover {
        case e: HttpRemoteProcessStarter.AgentException ⇒  // Error from Agent means the Agent is reachable
          Failure(FailableSelector.ExpectedException(e))
        case e @ (_: spray.can.Http.ConnectionException | _: AskTimeoutException) ⇒
          warningCall.call(e)
          Failure(e)
      }
    }

    def onDelay(delay: Duration, agent: Agent) = warningCall.call(null)
  }
  private[this] var agentSelector: AgentSelector = null
  private[this] var waitStopped = false
  private var remoteTaskClosed = false

  def close(): Unit = {
    if (agentSelector != null) {
      agentSelector.cancel()
    }
  }

  def startRemoteTask(failableAgents: FailableAgents): Unit = {
    agentSelector = startNewAgentSelector(failableAgents)
  }

  def changeFailableAgents(failableAgents: FailableAgents): Unit = {
    val a = agentSelector
    agentSelector.future.failed foreach { t ⇒
      logger.debug(s"$t")
      agentSelector = startNewAgentSelector(failableAgents)
    }
    a.cancel()
  }

  private def startNewAgentSelector(failableAgents: FailableAgents) = {
    val result = new AgentSelector(failableAgents, callbacks, callQueue)
    result.start() onComplete {
      case Success((agent, httpRemoteProcess)) ⇒
        logger.debug(s"Process on agent $agent started: $httpRemoteProcess")
        httpRemoteProcess.start()
        resultCall.call(Success(agent.address: String))
      case failure @ Failure(throwable) if waitStopped ⇒
        logger.debug(s"Waiting for agent has been stopped: $throwable")
        resultCall.call(failure)
      case failure @ Failure(_: FailableSelector.CancelledException) ⇒
        logger.debug(s"$failure")
      case failure @ Failure(throwable) ⇒
        if (result.isCancelled) {
          logger.debug(s"$failure")
        } else {
          logger.debug(s"Process on $result could not be started: $throwable", throwable)
          resultCall.call(failure)
        }
    }
    result
  }

  def stopTaskWaitingForAgent(): Unit = {
    waitStopped = true
    if (agentSelector != null) {
      agentSelector.cancel()
    }
  }

  @ForCpp
  def closeRemoteTask(kill: Boolean): Unit =
    if (!remoteTaskClosed) {
      killOrCloseRemoteTask(kill = kill, killOnlySignal = None)
    }

  @ForCpp
  def killRemoteTask(killOnlySignal: Int): Boolean = killOrCloseRemoteTask(kill = true, killOnlySignal = Some(killOnlySignal))

  def killOrCloseRemoteTask(kill: Boolean, killOnlySignal: Option[Int]): Boolean =
    agentSelector match {
      case null ⇒ false
      case _ ⇒
        agentSelector.cancel()
        agentSelector.future foreach { case (_, httpRemoteProcess) ⇒
          killOnlySignal match {
            case Some(signal) ⇒
              require(signal == SIGTERM.value, "SIGTERM (15) required")
              val whenSignalled = httpRemoteProcess.sendSignal(SIGTERM)
              for (t ← whenSignalled.failed) logger.error(s"Process '$httpRemoteProcess' on agent '$agentSelector' could not be signalled: $t", t)
            case None ⇒
              val whenClosed = httpRemoteProcess.closeRemoteTask(kill = kill)
              for (t ← whenClosed.failed) logger.error(s"Process '$httpRemoteProcess' on agent '$agentSelector' could not be closed: $t", t)
              whenClosed onComplete { _ ⇒ httpRemoteProcess.close() }
              remoteTaskClosed = true  // Do not execute remote_scheduler.remote_task.close twice!
          }
          // C++ will keine Bestätigung
        }
        true
    }

  @ForCpp
  override def toString = if (agentSelector != null) agentSelector.selectedString else "CppHttpRemoteApiProcessClient (not started)"
}

object CppHttpRemoteApiProcessClient {
  private type AgentSelector = FailableSelector[Agent, HttpRemoteProcess]
  private type FailableAgents = FailableCollection[Agent]

  private val logger = Logger(getClass)

  @Singleton
  final class Factory @Inject private(httpRemoteProcessStarter: Provider[HttpRemoteProcessStarter], callQueue: SchedulerThreadCallQueue) {
    def apply(apiProcessConfiguration: ApiProcessConfiguration, schedulerApiTcpPort: Int, warningCall: CppCall, resultCall: CppCall) =
      new CppHttpRemoteApiProcessClient(
        apiProcessConfiguration,
        schedulerApiTcpPort,
        warningCall = warningCall,
        resultCall = resultCall,
        httpRemoteProcessStarter.get(),
        callQueue)
  }
}
