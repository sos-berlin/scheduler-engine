package com.sos.scheduler.intern.cppproxy;

import com.sos.cplusplus.runtime.CppProxy;
import com.sos.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Job_subsystem", directory="scheduler", include="spooler.h")
public interface Job_subsystemC extends CppProxy {
    JobC job_by_string(String path);
    JobC job_by_string_or_null(String path);
    //String obj_name();
}
