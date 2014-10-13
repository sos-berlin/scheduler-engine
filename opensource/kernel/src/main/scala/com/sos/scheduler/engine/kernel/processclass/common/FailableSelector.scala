package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.common.async.{CallQueue, TimedCall}
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.kernel.processclass.common.FailableSelector._
import org.joda.time.{Duration, Instant}
import scala.concurrent.{Future, Promise}
import scala.util.control.NonFatal
import scala.util.{Failure, Success, Try}

/**
 * @author Joacim Zschimmer
 */
class FailableSelector[Failable, Result](
  failables: FailableCollection[Failable],
  callbacks: Callbacks[Failable, Result],
  callQueue: CallQueue) {

  import callQueue.implicits.executionContext

  private[this] var timedCall: TimedCall[Unit] = null
  @volatile private[this] var selected: Option[Failable] = None
  @volatile private[this] var cancelled = false
  private[this] val promise = Promise[(Failable, Result)]()

  final def start(): Future[(Failable, Result)] = {
    if (timedCall != null) throw new IllegalStateException("Single start only")
    def loopUntilConnected(): Unit = {
      val (delay, failable) = failables.nextDelayAndEntity()
      if (delay > 0.s) {
        callbacks.onDelay(delay, failable)
      }
      timedCall = callQueue.at(now + delay) {
        selected = Some(failable)
        catchInFuture { callbacks.apply(failable) } onComplete {
          case Success(Success(result)) ⇒
            failables.clearFailure(failable)
            promise.success(failable → result)
          case Success(Failure(throwable)) ⇒   // Tolerated failure
            failables.setFailure(failable, throwable)
            if (cancelled) {
              promise.failure(throwable)
            } else {
              loopUntilConnected()
            }
          case Failure(throwable) ⇒   // Failure lets abort FailableSelector
            failables.setFailure(failable, throwable)
            promise.failure(throwable)
        }
      }
    }
    loopUntilConnected()
    promise.future
  }

  final def cancel(): Unit = {
    cancelled = true
    for (o ← Option(timedCall)) {
      callQueue.tryCancel(o)
    }
  }

  final def future = promise.future

  final override def toString = s"FailableSelector ${selected getOrElse "(none)"}"

  protected def now = Instant.now()
}

object FailableSelector {
  trait Callbacks[Failable, Result] {
    /**
     * @return Future resulting in
     *         Success(Sucess(x)) => a selected,
     *         Success(Failure(t)) => a inaccessible,
     *         or Failure(t) => failure, abort
     */
    def apply(o: Failable): Future[Try[Result]]
    def onDelay(delay: Duration, a: Failable): Unit
  }

  private def catchInFuture[Result](body: ⇒ Future[Result]): Future[Result] =
    try body
    catch {
      case NonFatal(t) ⇒ Promise[Result]().failure(t).future
    }
}
