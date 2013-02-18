 package com.sos.scheduler.engine.kernel.async

 import com.sos.scheduler.engine.common.async.FutureCompletion.{callFuture, timedCallFuture}
 import java.lang.Thread.currentThread
 import org.joda.time.Instant
 import scala.concurrent.duration.Duration
 import scala.concurrent.{Await, Future}

 object SchedulerThreadFutures {

  def inSchedulerThread[A](f: => A)(implicit q: SchedulerThreadCallQueue): A =
    if (currentThread == q.thread) f
    else Await.result(schedulerThreadFuture(f), Duration.Inf)

  def schedulerThreadFuture[A](f: => A)(implicit callQueue: SchedulerThreadCallQueue): Future[A] =
    callFuture(f)

  def timedSchedulerThreadFuture[A](at: Instant)(f: => A)(implicit q: SchedulerThreadCallQueue): Future[A] =
    timedCallFuture(at)(f)
}
