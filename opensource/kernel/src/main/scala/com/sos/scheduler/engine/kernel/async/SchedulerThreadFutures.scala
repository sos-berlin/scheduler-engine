package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.FutureCompletion.{callFuture, timedCallFuture}
import java.lang.Thread.currentThread
import org.joda.time.Instant
import scala.concurrent.duration.Duration
import scala.concurrent.{Await, Future}
import scala.util.Try

object SchedulerThreadFutures {

  def inSchedulerThread[A](f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): A =
    Await.result(directOrSchedulerThreadFuture(f)(schedulerThreadCallQueue), Duration.Inf)

  /** Executes f, directly if in JobScheduler thread, else by CallQueue. */
  def directOrSchedulerThreadFuture[A](f: ⇒ A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): Future[A] =
    if (isInSchedulerThread)
      Future.fromTry(Try { f })
    else
      schedulerThreadFuture(f)(schedulerThreadCallQueue)

  def isInSchedulerThread(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue) =
    currentThread == schedulerThreadCallQueue.cppThread

  /** Future, der f als TimedCall in schedulerThreadCallQueue ausführt. */
  def schedulerThreadFuture[A](f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): Future[A] =
    callFuture(f)(schedulerThreadCallQueue)

  /** Future, der f als TimedCall in schedulerThreadCallQueue ausführt. */
  def timedSchedulerThreadFuture[A](at: Instant)(f: => A)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue): Future[A] =
    timedCallFuture(at)(f)
}
