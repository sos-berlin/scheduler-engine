package com.sos.scheduler.engine.kernel.job

import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.Task_subsystemC
import javax.inject.{Inject, Singleton}

@Singleton
final class TaskSubsystem @Inject private(
  cppProxy: Task_subsystemC,
  private[job] implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue) {

  private[kernel] def task(id: TaskId): Task =
    taskOption(id) getOrElse {
      throw new NoSuchElementException(s"Unknown TaskID '${id.string}'")
    }

  private[kernel] def taskOption(id: TaskId): Option[Task] =
    Option(cppProxy.get_task_or_null(id.number)) map { _.getSister }

  def taskLog(taskId: TaskId): String =
    try
      inSchedulerThread {
        cppProxy.task_log(taskId.number)
      }
    catch {
      case e: CppException if e.getCode == "SOS-1251" ⇒ throw new TaskNotFoundException(taskId)
    }
}
