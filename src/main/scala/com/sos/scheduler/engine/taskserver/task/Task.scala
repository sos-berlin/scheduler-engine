package com.sos.scheduler.engine.taskserver.task

/**
 * @author Joacim Zschimmer
 */
trait Task extends AutoCloseable {
  def start(): Unit
  def end(): Unit
  def step(): String
}
