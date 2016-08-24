package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.job.{TaskDetails, TaskId, TaskOverview}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.{directOrSchedulerThreadFuture, inSchedulerThread}
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

  def taskOverview(id: TaskId): Future[TaskOverview] = directOrSchedulerThreadFuture { subsystem.task(id).overview }

  def taskDetails(id: TaskId): Future[TaskDetails] = directOrSchedulerThreadFuture { subsystem.task(id).details }

  @deprecated("Avoid direct access to C++ near objects")
  def task(id: TaskId) = inSchedulerThread { subsystem.task(id) }
}
