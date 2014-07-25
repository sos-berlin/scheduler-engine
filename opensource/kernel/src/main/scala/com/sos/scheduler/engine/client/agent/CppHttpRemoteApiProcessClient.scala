package com.sos.scheduler.engine.client.agent

import com.google.inject.Injector
import com.sos.scheduler.engine.client.agent.CppHttpRemoteApiProcessClient._
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.Api_process_configurationC
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.util.{Failure, Success}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class CppHttpRemoteApiProcessClient private(starter: HttpRemoteProcessStarter, conf: ApiProcessConfiguration) {

  @volatile private var startFutureOption: Option[Future[HttpRemoteProcess]] = None
  @volatile private var processDescriptorOption: Option[ProcessDescriptor] = None
  @volatile private var state: State = Initialized

  @ForCpp
  def startRemoteTask(schedulerApiTcpPort: Int) {
    state = Starting
    val future = starter.startRemoteTask(schedulerApiTcpPort)
    future onComplete {
      case Success(o) ⇒
        processDescriptorOption = Some(o.processDescriptor)
        state = Started
        logger.info(s"Process on agent $agentUri started, processId: $o")
        // spooler_task.cxx continues when remote Task connects to waiting socket (com_remote.cxx)
      case Failure(t) ⇒
        state = StartFailed(t)
        logger.error(s"Process on agent $agentUri could not be started: $t")
        // After 60s timeout, spooler_task.cxx will reject task start with Z-REMOTE-118
    }
    startFutureOption = Some(future)
  }

  @ForCpp
  def closeRemoteTask() {
    closeRemoteTask(kill = false)
  }

  @ForCpp
  def killRemoteTask(): Boolean =
    closeRemoteTask(kill = true)

  private def closeRemoteTask(kill: Boolean): Boolean = {
    startFutureOption match {
      case None => false
      case Some(startFuture) ⇒
        state = Closing
        startFutureOption = None
        startFuture onSuccess {
          case httpRemoteProcess ⇒
            httpRemoteProcess.closeRemoteTask(kill = kill) onComplete {
              case Success(()) ⇒ state = Closed
              case Failure(t) ⇒ state = CloseFailed(t)
                logger.error(s"Process $processDescriptorOption on agent $agentUri could not be closed: $t")
            }
        }
        true
    }
  }

  @ForCpp
  override def toString = s"${getClass.getSimpleName}($agentUri, $state})"

  private def agentUri = conf.remoteSchedulerUri
}

object CppHttpRemoteApiProcessClient {
  private val logger = Logger(getClass)

  @ForCpp def apply(injector: Injector, conf: Api_process_configurationC) = {
    val c = ApiProcessConfiguration(conf)
    new CppHttpRemoteApiProcessClient(
      injector.apply[HttpRemoteProcessStarter.Factory].apply(c),
      c)
  }

  private sealed trait State
  private case object Initialized extends State
  private case object Starting extends State
  private case object Started extends State
  private case class StartFailed(throwable: Throwable) extends State
  private case object Closing extends State
  private case object Closed extends State
  private case class CloseFailed(throwable: Throwable) extends State
}
