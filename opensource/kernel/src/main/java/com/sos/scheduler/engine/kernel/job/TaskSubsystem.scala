package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.kernel.cppproxy.Task_subsystemC
import javax.inject.{Inject, Singleton}

@Singleton
final class TaskSubsystem @Inject private(cppProxy: Task_subsystemC) {

  def task(id: TaskId): Task =
    Option(cppProxy.get_task_or_null(id.value)) map { t => new Task(t) } getOrElse sys.error(s"Unknown TaskID '$id'")

  def taskLog(taskId: TaskId): String =
    try cppProxy.task_log(taskId.value)
    catch { case e: CppException if e.getCode == "SOS-1251" => throw new TaskNotFoundException(taskId) }
}
