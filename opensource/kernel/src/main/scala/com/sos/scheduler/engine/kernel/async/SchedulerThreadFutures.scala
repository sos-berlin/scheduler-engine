package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.FutureCompletion.{callFuture, timedCallFuture}
import java.lang.Thread.currentThread
import org.joda.time.Instant
import scala.concurrent.duration.Duration
import scala.concurrent.{Await, Future}

object SchedulerThreadFutures {

  def inSchedulerThread[A](f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): A =
    if (currentThread == schedulerThreadCallQueue.cppThread)
      f
    else
      Await.result(schedulerThreadFuture(f)(schedulerThreadCallQueue), Duration.Inf)   // Vielleicht mit close() in Scheduler gelöst: FIXME Bei Scheduler-Ende kann es noch passieren, dass die Future nicht endet. Tests terminieren damit nicht. Eigentlich sollte tryCancel() wirken.

  /** Future, der f als TimedCall in schedulerThreadCallQueue ausführt. */
  def schedulerThreadFuture[A](f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): Future[A] =
    callFuture(f)(schedulerThreadCallQueue)

  /** Future, der f als TimedCall in schedulerThreadCallQueue ausführt. */
  def timedSchedulerThreadFuture[A](at: Instant)(f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): Future[A] =
    timedCallFuture(at)(f)
}
