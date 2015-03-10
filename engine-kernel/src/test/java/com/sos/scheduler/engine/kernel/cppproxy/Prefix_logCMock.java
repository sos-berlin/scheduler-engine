package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl;
import com.sos.scheduler.engine.data.log.SchedulerLogLevel;
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

    public void java_log(int level, String line) {
        logger.debug(SchedulerLogLevel.ofCpp(level) + " " + line);
    }

    @Override public final String java_last(String log_level) {
        return "";
    }

    @Override public final boolean started() {
        return true;
    }

    @Override public final String this_filename() {
        return "";
    }
}
