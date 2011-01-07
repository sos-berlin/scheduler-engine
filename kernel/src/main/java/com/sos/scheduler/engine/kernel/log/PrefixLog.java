package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC;


public class PrefixLog {
    private Prefix_logC prefix_logC;
    
    public PrefixLog(Prefix_logC prefix_logC) {
        this.prefix_logC = prefix_logC;
    }
    
    public void info(String s) { prefix_logC.info(s); }
    public void warn(String s) { prefix_logC.warn(s); }
    public void error(String s) { prefix_logC.error(s); }
    public void debug3(String s) { prefix_logC.debug3(s); }
    public void debug(String s) { debug3(s); }
}
