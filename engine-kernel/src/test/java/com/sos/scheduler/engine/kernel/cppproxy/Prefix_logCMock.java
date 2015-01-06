package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import org.slf4j.Logger;

/** @author Zschimmer.sos */
public class Prefix_logCMock extends CppProxyImpl<PrefixLog> implements Prefix_logC {
    private final Logger logger;

    public Prefix_logCMock(Logger o) {
        logger = o;
    }

    @Override public boolean cppReferenceIsValid() { 
        return true;
    }

    @Override public void error(String line) { 
        logLine("ERROR", line);
    }

    @Override public void warn(String line) { 
        logLine("WARN", line);
    }

    @Override public void info(String line) { 
        logLine("info", line);
    }

    @Override public void debug3(String line) { 
        logLine("debug3", line);
    }

    @Override public String java_last(String log_level) {
        return "";
    }

    @Override public boolean started() {
        return true;
    }

    @Override public String this_filename() {
        return "";
    }

    private void logLine(String level, String line) {
        logger.debug(level +" "+ line);
    }
}
