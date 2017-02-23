package com.sos.scheduler.engine.kernel.job.internal

import com.sos.jobscheduler.common.scalautil.HasCloser
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
trait StandardAsynchronousJob extends HasCloser with AsynchronousJob {
  def start(): Future[Boolean] = Future.successful(true)
  def end(): Future[Unit] = Future.successful(())
  def step(): Future[Boolean]
}
