package com.sos.scheduler.engine.kernel.async

import com.sos.scheduler.engine.common.async.FutureCompletion.{callFuture, timedCallFuture}
import com.sos.scheduler.engine.common.time.ScalaJoda._
import java.lang.Thread.currentThread
import org.joda.time.Instant
import scala.concurrent.{Await, Future, TimeoutException}

object SchedulerThreadFutures {

  private val timeout = 30.s

  def inSchedulerThread[A](f: => A)(implicit q: SchedulerThreadCallQueue): A =
    if (currentThread == q.thread) f
    else {
      try Await.result(schedulerThreadFuture(f), timeout.toScalaDuration)
      catch {
        case e: TimeoutException =>
          throw new AssertionError(s"SchedulerThreadFutures.inSchedulerThread does not return within ${timeout.getMillis}ms: $e", e)
      }
    }

  def schedulerThreadFuture[A](f: => A)(implicit callQueue: SchedulerThreadCallQueue): Future[A] =
    callFuture(f)

  def timedSchedulerThreadFuture[A](at: Instant)(f: => A)(implicit q: SchedulerThreadCallQueue): Future[A] =
    timedCallFuture(at)(f)
}
