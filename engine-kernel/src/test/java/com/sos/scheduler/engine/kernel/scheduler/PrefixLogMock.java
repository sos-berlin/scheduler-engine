package com.sos.scheduler.engine.kernel.scheduler;

import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logCMock;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import org.slf4j.Logger;

public class PrefixLogMock {
    private PrefixLogMock() {}
    
    public static PrefixLog newLog(Logger o) {
        return new PrefixLog(new Prefix_logCMock(o));
    }
}
