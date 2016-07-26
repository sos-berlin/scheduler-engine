package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.job.{TaskId, TaskOverview}
import com.sos.scheduler.engine.kernel.async.{SchedulerThreadCallQueue, SchedulerThreadFutures}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.{inSchedulerThread, schedulerThreadFuture}
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class TaskSubsystemClient @Inject private(
  protected val subsystem: TaskSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
{

  def taskOverview(id: TaskId): Future[TaskOverview] = schedulerThreadFuture { subsystem.task(id).overview }

  @deprecated("Avoid direct access to C++ near objects")
  def task(id: TaskId) = inSchedulerThread { subsystem.task(id) }
}
