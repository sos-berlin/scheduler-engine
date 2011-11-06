package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Prefix_log", directory="scheduler", include="spooler.h")
public interface Prefix_logC extends CppProxy {
    void info(String line);
    void warn(String line);
    void error(String line);
    void debug3(String line);
    String java_last(String log_level);
}
