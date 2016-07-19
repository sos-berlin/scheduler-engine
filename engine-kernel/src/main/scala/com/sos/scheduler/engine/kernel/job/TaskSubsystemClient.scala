package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.job.{TaskId, TaskOverview}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class TaskSubsystemClient @Inject private(
  protected val subsystem: TaskSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
{

  def taskOverview(id: TaskId): TaskOverview = inSchedulerThread { subsystem.task(id).overview }

  @deprecated("Avoid direct access to C++ near objects")
  def task(id: TaskId) = inSchedulerThread { subsystem.task(id) }
}
