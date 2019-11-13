package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import com.sos.scheduler.engine.kernel.monitor.{MonitorSister, MonitorSubsystem}

@CppClass(clas = "sos::scheduler::Monitor_subsystem", directory = "scheduler", include = "spooler.h")
trait Monitor_subsystemC extends SubsystemC[MonitorSister, MonitorC]
with CppProxyWithSister[MonitorSubsystem]

object Monitor_subsystemC
{
  val sisterType = MonitorSubsystem.Type
}
