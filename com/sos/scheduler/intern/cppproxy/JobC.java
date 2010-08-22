package com.sos.scheduler.intern.cppproxy;

import com.sos.cplusplus.runtime.CppProxy;
import com.sos.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Job", directory="scheduler", include="spooler.h")
public interface JobC extends CppProxy {
    String name();
    String path();
    //String obj_name();
}
