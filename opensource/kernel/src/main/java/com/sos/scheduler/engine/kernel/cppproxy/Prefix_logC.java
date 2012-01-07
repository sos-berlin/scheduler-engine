package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.log.PrefixLog;

@CppClass(clas="sos::scheduler::Prefix_log", directory="scheduler", include="spooler.h")
public interface Prefix_logC extends CppProxyWithSister<PrefixLog> {
    PrefixLog.Type sisterType = new PrefixLog.Type();

    void info(String line);
    void warn(String line);
    void error(String line);
    void debug3(String line);
    String java_last(String log_level);
    String filename();
}
