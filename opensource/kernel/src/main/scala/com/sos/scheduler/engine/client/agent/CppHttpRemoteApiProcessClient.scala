package com.sos.scheduler.engine.client.agent

import com.google.inject.Injector
import com.sos.scheduler.engine.client.agent.CppHttpRemoteApiProcessClient._
import com.sos.scheduler.engine.common.async.TimedCall
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.cppproxy.Api_process_configurationC
import java.util.concurrent.atomic.AtomicReference
import org.joda.time.Instant.now
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.util.{Failure, Success}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class CppHttpRemoteApiProcessClient private(
  starter: HttpRemoteProcessStarter,
  callQueue: SchedulerThreadCallQueue,
  conf: ApiProcessConfiguration) {

  private val startFutureAtomic = new AtomicReference[Future[HttpRemoteProcess]]
  private val timedCallAtomic = new AtomicReference[TimedCall[Unit]]
  @volatile private var processDescriptorOption: Option[ProcessDescriptor] = None
  @volatile private var state: State = Initialized

  @ForCpp
  def startRemoteTask(schedulerApiTcpPort: Int, waitingCall: CppCall, resultCall: CppCall): Unit = {
    state = Starting
    def loop(): Unit = {
      val future = starter.startRemoteTask(schedulerApiTcpPort)
      startFutureAtomic.set(future)
      future onComplete {
        case success@Success(o) ⇒
          processDescriptorOption = Some(o.processDescriptor)
          state = Started
          logger.info(s"Process on agent $agentUri started, processId: $o")
          resultCall.call(success)
        case failure@Failure(throwable) ⇒
          throwable match {
            case e: spray.can.Http.ConnectionAttemptFailedException if state == Starting ⇒
              waitingCall.call(e)
              timedCallAtomic set callQueue.at(now + TryConnectInterval) {
                loop()
              }
            case _ ⇒
              state = StartFailed(throwable)
              logger.debug(s"Process on agent $agentUri could not be started: $throwable")
              resultCall.call(failure)
          }
      }
    }
    loop()
  }

  @ForCpp
  def closeRemoteTask(): Unit =
    closeRemoteTask(kill = false)

  @ForCpp
  def killRemoteTask(): Boolean =
    closeRemoteTask(kill = true)

  private def closeRemoteTask(kill: Boolean): Boolean = {
    for (o ← Some(timedCallAtomic.get)) {
      callQueue.tryCancel(o)
    }
    val startFuture = startFutureAtomic.getAndSet(null)
    if (startFuture == null) {
      state = Closed
      false
    } else {
      // TODO Race condition? Wenn das <start>-Kommando gerade rübergeschickt wird, startet der Prozess und verbindet sich
      // mit einem com_remote-Port. Der aber ist vielleicht schon von der nächsten Task, zu gerade gestartet wird.
      // Wird die dann gestört? Wenigstens ist es wenig wahrscheinlich.
      state = Closing
      startFuture onSuccess {
        case httpRemoteProcess ⇒
          httpRemoteProcess.closeRemoteTask(kill = kill) onComplete {
            case Success(()) ⇒
              state = Closed
            case Failure(t) ⇒
              state = CloseFailed(t)
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
  private val TryConnectInterval = 5.s
  private val logger = Logger(getClass)

  @ForCpp def apply(injector: Injector, conf: Api_process_configurationC) = {
    val c = ApiProcessConfiguration(conf)
    new CppHttpRemoteApiProcessClient(
      injector.apply[HttpRemoteProcessStarter.Factory].apply(c),
      injector.apply[SchedulerThreadCallQueue],
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
