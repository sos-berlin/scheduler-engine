package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.job.Job;

@CppClass(clas="sos::scheduler::Job_subsystem", directory="scheduler", include="spooler.h")
public interface Job_subsystemC extends SubsystemC<Job, JobC>, CppProxy {
}
