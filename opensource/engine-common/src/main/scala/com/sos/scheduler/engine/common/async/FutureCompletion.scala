package com.sos.scheduler.engine.common.async

import org.joda.time.Instant
import scala.concurrent.{Future, Promise}
import scala.util.Try

trait FutureCompletion[A] {
  this: TimedCall[A] =>

  private val promise = Promise[A]()

  override protected def onComplete(result: Try[A]) {
    promise complete result
  }

  final def future = promise.future
}

object FutureCompletion {
  type FutureCall[A] = TimedCall[A] with FutureCompletion[A]

  def futureCall[A](f: => A): FutureCall[A] =
    futureTimedCall(TimedCall.shortTerm)(f)

  def futureTimedCall[A](at: Instant)(f: => A): FutureCall[A] =
    new TimedCall[A] with FutureCompletion[A] {
      def epochMillis = at.getMillis
      def call() = f
    }

  def callFuture[A](f: => A)(implicit callQueue: CallQueue): Future[A] =
    timedCallFuture(TimedCall.shortTerm)(f)

  def timedCallFuture[A](at: Instant)(f: => A)(implicit callQueue: CallQueue): Future[A] = {
    val call = futureTimedCall(at)(f)
    callQueue add call
    call.future
  }
}