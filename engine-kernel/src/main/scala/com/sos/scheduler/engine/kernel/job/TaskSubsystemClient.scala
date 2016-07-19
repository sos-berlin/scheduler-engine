package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.job.TaskId
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

  def task(id: TaskId) =
    inSchedulerThread {
      subsystem.task(id)
    }
}
