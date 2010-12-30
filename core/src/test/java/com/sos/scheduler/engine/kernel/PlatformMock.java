package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC;
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logCMock;
import java.io.PrintWriter;
import java.io.Writer;


public class PlatformMock {
    public static Platform newInstance(Writer w) {
        Prefix_logC prefix_logC = new Prefix_logCMock(w);
        PrefixLog log = new PrefixLog(prefix_logC);
        return new Platform(log);
    }

    public static Platform newInstance() { return newInstance(new PrintWriter(System.err)); }
}
