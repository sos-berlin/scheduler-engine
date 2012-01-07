package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC;

import java.io.File;

@ForCpp
public final class PrefixLog implements Sister, SchedulerLogger {
    private final Prefix_logC cppProxy;
    
    public PrefixLog(Prefix_logC cppProxy) {
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {

    }
    @Override public void info(String s) {
        cppProxy.info(s);
    }

    @Override public void warn(String s) {
        cppProxy.warn(s);
    }

    @Override public void error(String s) {
        cppProxy.error(s);
    }

    public void debug3(String s) {
        cppProxy.debug3(s);
    }

    @Override public void debug(String s) {
        debug3(s);
    }

    /** @return "", wenn für den Level keine Meldung vorliegt. */
    public String lastByLevel(SchedulerLogLevel level) {
        return cppProxy.java_last(level.getCppName());
    }

    public File getFile() {
        return new File(cppProxy.filename());
    }

    public static class Type implements SisterType<PrefixLog, Prefix_logC> {
        @Override public final PrefixLog sister(Prefix_logC proxy, Sister context) {
            return new PrefixLog(proxy);
        }
    }
}
