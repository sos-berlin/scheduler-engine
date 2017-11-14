package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.base.utils.ScalaUtils.functionWithToString
import com.sos.scheduler.engine.common.async.FutureCompletion.functionToFutureTimedCall
import com.sos.scheduler.engine.common.async.{CallQueue, TimedCall}
import com.sos.scheduler.engine.common.scalautil.Futures.catchInFuture
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.kernel.processclass.common.FailableSelector._
import java.time.{Duration, Instant}
import scala.concurrent.{Future, Promise}
import scala.util.{Failure, Success, Try}

/**
 * @author Joacim Zschimmer
 */
class FailableSelector[Failable, Result](
  failables: FailableCollection[Failable],
  callbacks: Callbacks[Failable, Result],
  callQueue: CallQueue,
  connectionTimeout: Option[Duration]) {

  import callQueue.implicits.executionContext

  @volatile private[this] var timedCall: TimedCall[Unit] = null
  @volatile private[this] var selected: Option[Failable] = None
  @volatile private[this] var cancelled = false
  private[this] val promise = Promise[(Failable, Result)]()

  final def start(): Future[(Failable, Result)] = {
    val connectUntil = connectionTimeout map now.+
    if (timedCall != null) throw new IllegalStateException("Single start only")
    def loopUntilConnected(): Unit = {
      val (delay, failable) = nextDelayAndEntity(connectUntil)
      if (delay > 0.s) {
        callbacks.onDelay(delay, failable)
      }
      val t = functionToFutureTimedCall[Unit](now + delay, functionWithToString(toString) {
        selected = Some(failable)
        catchInFuture { callbacks.apply(failable).appendCurrentStackTrace } onComplete {
          case Success(Success(result)) ⇒
            failables.clearFailure(failable)
            promise.success(failable → result)
          case x if cancelled ⇒
            logger.debug(s"$x")
            promise.failure(new CancelledException)
          case Success(Failure(ExpectedException(e))) ⇒
            promise.failure(e)
          case Success(Failure(throwable)) ⇒
            failables.setFailure(failable, throwable)
            if (connectUntil exists (now >= _)) {
              logger.debug(s"Failing after connectionTimeout=${connectionTimeout.get.pretty}")
              promise.failure(throwable)
            } else {
              loopUntilConnected()  // Tolerated failure
            }
          case f @ Failure(_: TimedCall.CancelledException) ⇒
            logger.debug(s"$f")
            promise.failure(new CancelledException)
          case Failure(throwable) ⇒ // Failure lets abort FailableSelector
            failables.setFailure(failable, throwable)
            promise.failure(throwable)
        }
      })
      callQueue.add(t)
      timedCall = t
      for (throwable ← t.future.appendCurrentStackTrace.failed) {
        val x = throwable match {
          case _: TimedCall.CancelledException ⇒ new CancelledException
          case _ ⇒ throwable
        }
        promise.tryFailure(x)
      }
    }
    loopUntilConnected()
    promise.future
  }

  private final def nextDelayAndEntity(connectUntil: Option[Instant]): (Duration, Failable) = {
    val (delay, failable) = failables.nextDelayAndEntity()
    val d = connectUntil match {
      case Some(until) ⇒ (until - now) max 0.s min delay
      case None ⇒ delay
    }
    (d, failable)
  }

  final def cancel(): Unit = {
    cancelled = true
    for (o ← Option(timedCall)) {
      callQueue.tryCancel(o)
    }
  }

  final def future = promise.future

  final def isCancelled = cancelled

  final override def toString = s"FailableSelector $selectedString"

  final def selectedString: String = s"${selected getOrElse "(none)"}" + (if (cancelled) " cancelled" else "")

  protected def now = Instant.now()
}

object FailableSelector {
  private val logger = Logger(getClass)

  trait Callbacks[Failable, Result] {
    /**
     * @return Future resulting in<br/>
     *         Success(Success(x)) => a selected,<br/>
     *         Success(Failure(t)) => a inaccessible,<br/>
     *         or Failure(t) => failure, abort
     */
    def apply(o: Failable): Future[Try[Result]]
    def onDelay(delay: Duration, a: Failable): Unit
  }

  final class CancelledException private[FailableSelector] extends RuntimeException

  final case class ExpectedException(throwable: Throwable) extends RuntimeException(throwable)
}
