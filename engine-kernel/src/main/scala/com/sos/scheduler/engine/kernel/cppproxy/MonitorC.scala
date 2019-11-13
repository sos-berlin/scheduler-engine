package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import com.sos.scheduler.engine.kernel.monitor.MonitorSister

@CppClass(clas = "sos::scheduler::Monitor", directory = "scheduler", include = "spooler.h")
trait MonitorC  extends CppProxyWithSister[MonitorSister] with File_basedC[MonitorSister]

object MonitorC {
  val sisterType = MonitorSister.Type
}
