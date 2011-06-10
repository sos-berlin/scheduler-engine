package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC;


@ForCpp
public final class PrefixLog implements SchedulerLogger {
    private final Prefix_logC cppProxy;
    
    public PrefixLog(Prefix_logC cppProxy) {
        this.cppProxy = cppProxy;
    }
    
    @Override public final void info(String s) { cppProxy.info(s); }
    @Override public final void warn(String s) { cppProxy.warn(s); }
    @Override public final void error(String s) { cppProxy.error(s); }
    public final void debug3(String s) { cppProxy.debug3(s); }
    @Override public final void debug(String s) { debug3(s); }
}
