package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC;
import org.apache.log4j.Logger;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

@ForCpp
public final class PrefixLog implements Sister, SchedulerLogger {
    private static final Logger logger = Logger.getLogger(PrefixLog.class);

    private final Prefix_logC cppProxy;
    private final List<LogSubscription> subscriptions = new ArrayList<LogSubscription>();
    private final LogSubscription[] emptyLogSubscriptions = new LogSubscription[0];
    private LogSubscription[] subscriptionsSnapshot = emptyLogSubscriptions;
    private boolean subscriptionsModified = false;
//    private File fileToBeRemoved = null;

    public PrefixLog(Prefix_logC cppProxy) {
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {}

    @ForCpp public void onStarted() {
        for (LogSubscription o: subscriptionsSnapshot()) {
            try {
                o.onStarted();
            } catch (Throwable t) { logger.error(o+": "+t, t); }
        }
    }

    @ForCpp public void onClosed() {
        for (LogSubscription o: subscriptionsSnapshot()) {
            try {
                o.onClosed();
            } catch (Throwable t) { logger.error(o+": "+t, t); }
        }
        subscriptions.clear();
    }

    @ForCpp public void onLogged() {
        for (LogSubscription o: subscriptionsSnapshot()) {
            try {
                o.onLogged();
            } catch (Throwable t) { logger.error(o+": "+t, t); }
        }
    }

//    @ForCpp public void removeFile(String file) {
//        fileToBeRemoved = new File(file);
//    }

//    private void removeFileNow() {
//        if (fileToBeRemoved != null) {
//            boolean ok = fileToBeRemoved.delete();
//            if (!ok)
//                logger.error("SCHEDULER-291 "+fileToBeRemoved);
//        }
//    }

    private LogSubscription[] subscriptionsSnapshot() {
        synchronized (subscriptions) {
            if (subscriptionsModified) {
                subscriptionsModified = false;
                subscriptionsSnapshot = subscriptions.isEmpty()?
                        emptyLogSubscriptions : subscriptions.toArray(new LogSubscription[subscriptions.size()]);
            }
        }
        return subscriptionsSnapshot;
    }

    public void subscribe(LogSubscription o) {
        synchronized (subscriptions) {
            subscriptions.add(o);
            subscriptionsModified = true;
        }
    }

    public void unsubscribe(LogSubscription o) {
        synchronized (subscriptions) {
            subscriptions.remove(o);
            subscriptionsModified = true;
//            if (subscriptions.isEmpty())
//                removeFileNow();
        }
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

    public boolean isStarted() {
        return cppProxy.started();
    }

    public File getFile() {
        return new File(cppProxy.this_filename());
    }

    public static class Type implements SisterType<PrefixLog, Prefix_logC> {
        @Override public final PrefixLog sister(Prefix_logC proxy, Sister context) {
            return new PrefixLog(proxy);
        }
    }
}
