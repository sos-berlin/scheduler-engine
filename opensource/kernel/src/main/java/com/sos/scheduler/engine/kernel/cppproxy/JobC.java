package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.job.Job;

@CppClass(clas="sos::scheduler::Job", directory="scheduler", include="spooler.h")
public interface JobC extends CppProxyWithSister<Job> {
    Job.Type sisterType = new Job.Type();

    String file_based_state_name();
    boolean is_file_based_reread();
    String name();
    String path();
    Prefix_logC log();
}
