package com.sos.scheduler.engine.kernel.processclass.agent

import com.sos.scheduler.engine.client.agent.{ApiProcessConfiguration, HttpRemoteProcess, HttpRemoteProcessStarter}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.processclass.agent.CppHttpRemoteApiProcessClient._
import com.sos.scheduler.engine.kernel.processclass.common.{FailableCollection, FailableSelector}
import javax.inject.Inject
import org.joda.time.Duration
import scala.concurrent.Future
import scala.util.{Failure, Success}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class CppHttpRemoteApiProcessClient private(
  failableAgents: FailableCollection[Agent],
  apiProcessConfiguration: ApiProcessConfiguration,
  starter: HttpRemoteProcessStarter,
  callQueue: SchedulerThreadCallQueue) {

  import callQueue.implicits.executionContext

  @volatile private[this] var agentSelector: FailableSelector[Agent, HttpRemoteProcess] = null

  @ForCpp
  def startRemoteTask(schedulerApiTcpPort: Int, warningCall: CppCall, resultCall: CppCall): Unit = {
    val callbacks = new FailableSelector.Callbacks[Agent, HttpRemoteProcess] {
      def apply(agent: Agent) = {
        val future = starter.startRemoteTask(schedulerApiTcpPort, apiProcessConfiguration, remoteUri = agent.address)
        future map Success.apply recoverWith {
          case e: spray.can.Http.ConnectionAttemptFailedException ⇒
            warningCall.call(e)
            Future { Failure(e) }
        }
      }

      def onDelay(delay: Duration, agent: Agent) =
        warningCall.call(null)
    }
    agentSelector = new FailableSelector(failableAgents, callbacks, callQueue)
    agentSelector.start() onComplete {
      case Success((agent, httpRemoteProcess)) ⇒
        logger.debug(s"Process on agent $agent started: $httpRemoteProcess")
        resultCall.call(Success(()))
      case Failure(throwable) ⇒
        logger.debug(s"Process on $agentSelector could not be started: $throwable")
        resultCall.call(Failure(throwable))
    }
  }

  @ForCpp
  def closeRemoteTask(): Unit =
    closeRemoteTask(kill = false)

  @ForCpp
  def killRemoteTask(): Boolean =
    closeRemoteTask(kill = true)

  private def closeRemoteTask(kill: Boolean): Boolean = {
    agentSelector match {
      case null ⇒ false
      case _ ⇒
        agentSelector.cancel()
        // TODO Race condition? Wenn das <start>-Kommando gerade rübergeschickt wird, startet der Prozess und verbindet sich
        // mit einem com_remote-Port. Der aber ist vielleicht schon von der nächsten Task, zu gerade gestartet wird.
        // Wird die dann gestört? Wenigstens ist es wenig wahrscheinlich.
        agentSelector.future onSuccess {
          case (agent, httpRemoteProcess) ⇒
            httpRemoteProcess.closeRemoteTask(kill = kill) onFailure {
              case t ⇒ logger.error(s"Process $httpRemoteProcess on agent $agentSelector could not be closed: $t")
              // C++ will keine Bestätigung
            }
        }
        true
    }
  }

  @ForCpp
  override def toString = if (agentSelector != null) agentSelector.toString else "CppHttpRemoteApiProcessClient"
}

object CppHttpRemoteApiProcessClient {
  private val logger = Logger(getClass)

  final class Factory @Inject private(httpRemoteProcessStarter: HttpRemoteProcessStarter, callQueue: SchedulerThreadCallQueue) {
    def apply(failableAgents: FailableCollection[Agent], conf: ApiProcessConfiguration) =
      new CppHttpRemoteApiProcessClient(failableAgents, conf, httpRemoteProcessStarter, callQueue)
  }
}
