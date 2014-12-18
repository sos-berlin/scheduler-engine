package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Logger

/**
 * @author Joacim Zschimmer
 */
trait Task {
  def start(): Boolean
  def end()
  def step(): Any
}

private object Task {
  private val logger = Logger(getClass)
}
