package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Logger

/**
 * @author Joacim Zschimmer
 */
trait Task extends AutoCloseable {
  def start(): Unit
  def end(): Unit
  def step(): Any
}

private object Task {
  private val logger = Logger(getClass)
}
