package com.sos.scheduler.engine.playground.plugins.jobnet.scheduler

object Transaction {
  def transaction[A](f: => A) = f
}
