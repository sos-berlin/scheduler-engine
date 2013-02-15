package com.sos.scheduler.engine.common.async

import scala.concurrent.Promise
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
  def futureCall[A](at: Long = TimedCall.shortTerm)(f: => A) = new AbstractTimedCall[A](at) with FutureCompletion[A] {
    def call() = f
  }

//  def futureCall[A]()(f: => A) = new ShortTermCall[A] with FutureCompletion[A] {
//    def call() = f
//  }
}