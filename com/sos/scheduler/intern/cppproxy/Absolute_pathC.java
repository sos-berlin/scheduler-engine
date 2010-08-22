package com.sos.scheduler.intern.cppproxy;

import com.sos.cplusplus.runtime.CppProxy;
import com.sos.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Absolute_path", directory="scheduler", include="spooler.h")
public interface Absolute_pathC extends CppProxy {
    JobC job(String path);
    JobC job_or_null(String path);
    //String obj_name();
}
