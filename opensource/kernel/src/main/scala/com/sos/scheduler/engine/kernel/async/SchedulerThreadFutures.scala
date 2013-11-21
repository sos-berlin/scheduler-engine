package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.FutureCompletion.{callFuture, timedCallFuture}
import java.lang.Thread.currentThread
import org.joda.time.Instant
import scala.concurrent.duration.Duration
import scala.concurrent.{Await, Future}

object SchedulerThreadFutures {

  // FIXME Bei Scheduler-Ende kann es noch passieren, dass die Future nicht endet. Tests terminieren damit nicht. Eigentlich sollte tryCancel() wirken.

  def inSchedulerThread[A](f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): A =
    if (currentThread == schedulerThreadCallQueue.thread) f
    else {
      val future = schedulerThreadFuture(f)
      Await.ready(future, Duration.Inf)
      future.value.get.get
    }

  def schedulerThreadFuture[A](f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): Future[A] =
    callFuture(f)(schedulerThreadCallQueue)

  def timedSchedulerThreadFuture[A](at: Instant)(f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): Future[A] =
    timedCallFuture(at)(f)
}
