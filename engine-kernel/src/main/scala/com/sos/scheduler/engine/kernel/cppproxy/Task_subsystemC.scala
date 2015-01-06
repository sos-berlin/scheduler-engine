package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import javax.annotation.Nullable

@CppClass(clas = "sos::scheduler::Task_subsystem", directory = "scheduler", include = "spooler.h")
trait Task_subsystemC extends CppProxy {

  def task_log(taskId: Int): String

  @Nullable
  def get_task_or_null(taskId: Int): TaskC
}
