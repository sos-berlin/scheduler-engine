package com.sos.scheduler.engine.kernel.job.internal

import com.sos.scheduler.engine.kernel.job.Task
import scala.concurrent.Future

/**
 * Simplified and asynchronous interface for a job implementation (like sos.spooler.Job_impl).
 *
 * @author Joacim Zschimmer
 */
trait AsynchronousJob extends AutoCloseable {

  protected val task: Task

  def start(): Future[Boolean]

  def end(): Future[Unit]

  def step(): Future[Boolean]

  /**
   * Do not block!
   */
  def close(): Unit
}
