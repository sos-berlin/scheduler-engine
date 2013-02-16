package com.sos.scheduler.engine.cplusplus.runtime;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.annotation.Nullable;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.ReentrantLock;

import static java.lang.System.currentTimeMillis;
import static java.lang.Thread.currentThread;

public class ThreadLock {
    private static final int logTimeoutMillis = 30*1000;     // Wenn's länger dauert, Meldung loggen
    private static final Logger logger = LoggerFactory.getLogger(ThreadLock.class);

    private final SimpleLock myLock = new LoggingLock();
//    private final AtomicInteger counter = new AtomicInteger(0);

    public final void lock() {
        myLock.lock();
    }

    public final void unlock() {
        myLock.unlock();
    }

    @Override public String toString() {
        return myLock.toString();
    }

    private class SimpleLock {
        private final ReentrantLock lock = new ReentrantLock();

        void lock() {
            lock.lock();
            onLocked();
        }

        final boolean tryLock(int time, TimeUnit unit) {
            try {
                boolean locked = lock.tryLock(time, unit);
                if (locked)
                    onLocked();
                return locked;
            }
            catch (InterruptedException x) { throw new RuntimeException(x); }
        }

        protected void onLocked() {}

        void unlock() {
            lock.unlock();
        }

        @Override public String toString() {
            return SimpleLock.class.getName()+"("+lock+")";
        }
    }


    private final class LoggingLock extends SimpleLock {
        @Nullable private final AtomicReference<Thread> lockingThread = new AtomicReference<Thread>();  // Ungenau wegen race condition
//        private Locked locked = null;
//        private final WatchdogThread watchdogThread = new WatchdogThread(this);

        @Override void lock() {
            long t = currentTimeMillis();
            boolean locked = tryLock(logTimeoutMillis, TimeUnit.MILLISECONDS);
            if (!locked) {
                logger.warn("Waiting for Scheduler ThreadLock, currently acquired by "+lockingThreadString());
                super.lock();
                logger.warn("Scheduler ThreadLock acquired after waiting {}ms", t - currentTimeMillis());
            }
        }

        private String lockingThreadString() {
            Thread t = lockingThread.get();
            if (t == null) {
                return "(unknown)";
            } else {
                StringWriter stringWriter = new StringWriter();
                PrintWriter w = new PrintWriter(stringWriter);
                w.print(t.toString());
                w.print(", current stack trace:\n");
                Exception x = new Exception() {};
                x.setStackTrace(t.getStackTrace());
                x.printStackTrace(w);
                w.flush();
                return stringWriter.toString();
            }
        }

        protected void onLocked() {
            lockingThread.set(currentThread());
            //locked = new Locked(counter.addAndGet(1));
        }

        @Override void unlock() {
            lockingThread.set(null);
            super.unlock();
        }

//        private class Locked {
//            private final int id;
//            private final long since;
//
//            private Locked(int id) {
//                this.id = id;
//                since = currentTimeMillis();
//            }
//        }

//        private final class WatchdogThread extends Thread {
//            private final LoggingLock loggingLock;
//
//            private WatchdogThread(LoggingLock l) {
//                loggingLock = l;
//            }
//
//            @Override public void run() {
//                try {
//                    while (true) {
//                        sleep(1000);
//                        Locked locked = loggingLock.locked;
//                        if (locked != null && locked.since + logTimeoutMillis > currentTimeMillis())
//                            logger.warn("Scheduler ThreadLock acquired since {}ms by {}", currentTimeMillis() - locked.since, loggingLock.callersData);
//                    }
//                } catch (InterruptedException ignore) {}
//            }
//        }
    }
}
