package com.sos.scheduler.intern.cppproxy;

import com.sos.cplusplus.runtime.CppProxy;
import com.sos.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Prefix_log", directory="scheduler", include="spooler.h")
public interface Prefix_logC extends CppProxy {
    void info(String line);
    void warn(String line);
    void error(String line);
    void debug3(String line);
    //String obj_name();
}
