package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.{CppClass, CppExpression}
import com.sos.scheduler.engine.kernel.job.Task

@CppClass(clas="sos::scheduler::Task", directory="scheduler", include="spooler.h")
trait TaskC extends CppProxyWithSister[Task] {
  def id: Int
  def job: JobC
  def job_path: String
  def state_name: String
  def remote_scheduler_address: String
  def order: OrderC
  def params: Variable_setC
  def log_string: String
  def stdout_path: String
  def stderr_path: String
  def log: Prefix_logC

  @CppExpression("$->process_started_at().millis()")
  def processStartedAt: Long

  @CppExpression("$->step_or_process_started_at().millis()")  // First step starts with process, second with step (spooler_process)
  def stepOrProcessStartedAt: Long

  def process_class_or_null: Process_classC

  def is_waiting_for_remote_scheduler: Boolean

  @CppExpression("$->at().millis()")
  def at_millis: Long
}

object TaskC {
  val sisterType = Task.Type
}
