package com.sos.scheduler.engine.common.async

trait ShortTermCall[A] extends TimedCall[A] {
  final def at = TimedCall.shortTerm
}

object ShortTermCall {
  def apply[A](f: => A) = new ShortTermCall[A] {
    def call() = f
  }
}
