package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC;

@ForCpp
public class PrefixLog implements SchedulerLogger {
    private Prefix_logC prefix_logC;
    
    public PrefixLog(Prefix_logC prefix_logC) {
        this.prefix_logC = prefix_logC;
    }
    
    @Override public void info(String s) { prefix_logC.info(s); }
    @Override public void warn(String s) { prefix_logC.warn(s); }
    @Override public void error(String s) { prefix_logC.error(s); }
    public void debug3(String s) { prefix_logC.debug3(s); }
    @Override public void debug(String s) { debug3(s); }
}
