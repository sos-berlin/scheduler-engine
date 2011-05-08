package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.cplusplus.runtime.*;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Job", directory="scheduler", include="spooler.h")
public interface JobC extends CppProxyWithSister<Job>
{
    Job.Type sisterType = new Job.Type();

    String name();
    String path();
    //String obj_name();
}
