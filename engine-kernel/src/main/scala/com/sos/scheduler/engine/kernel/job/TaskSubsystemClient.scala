package com.sos.scheduler.engine.kernel.job

import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.job.{TaskDetailed, TaskOverview}
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

  def taskDetailed(id: TaskId): Future[TaskDetailed] = directOrSchedulerThreadFuture { subsystem.task(id).details }

  @deprecated("Avoid direct access to C++ near objects")
  def task(id: TaskId) = inSchedulerThread { subsystem.task(id) }
}
