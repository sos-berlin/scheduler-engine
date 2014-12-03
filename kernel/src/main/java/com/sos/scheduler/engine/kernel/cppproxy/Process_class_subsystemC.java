package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.processclass.ProcessClass;

@CppClass(clas="sos::scheduler::Process_class_subsystem", directory="scheduler", include="spooler.h")
public interface Process_class_subsystemC extends SubsystemC<ProcessClass, Process_classC>, CppProxy {}
