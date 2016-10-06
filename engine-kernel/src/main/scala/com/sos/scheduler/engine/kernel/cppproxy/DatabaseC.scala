package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass

@CppClass(clas = "sos::scheduler::Database", directory = "scheduler", include = "spooler.h")
trait DatabaseC extends CppProxy {

  def properties: Variable_setC // Mit "password"

  def jdbc_connection: AnyRef

  def transform_sql(statement: String): String
}
