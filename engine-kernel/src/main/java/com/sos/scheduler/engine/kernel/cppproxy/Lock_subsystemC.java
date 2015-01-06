package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.lock.Lock;

@CppClass(clas="sos::scheduler::lock::Lock_subsystem", directory="scheduler", include="spooler.h")
public interface Lock_subsystemC extends SubsystemC<Lock, LockC>, CppProxy {}
