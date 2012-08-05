package com.sos.scheduler.engine.cplusplus.runtime;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;

import javax.annotation.Nullable;

public class ThreadLock {
    private static final int logTimeoutMillis = 30*1000;     // Wenn's länger dauert, Meldung loggen
    private static final Level logLevel = Level.WARN;
    private static final Logger logger = Logger.getLogger(ThreadLock.class);

    private final SimpleLock myLock = logger.isEnabledFor(logLevel)? new LoggingLock() : new SimpleLock();

    public final void lock() {
        myLock.lock();
    }

    public final void unlock() {
        myLock.unlock();
    }

    @Override public String toString() {
        return myLock.toString();
    }

    private static class SimpleLock {
        private final ReentrantLock lock = new ReentrantLock();

        void lock() {
            lock.lock();
        }

        final boolean tryLock(int time, TimeUnit unit) {
            try {
                return lock.tryLock(time, unit);
            }
            catch (InterruptedException x) { throw new RuntimeException(x); }
        }

        void unlock() {
            lock.unlock();
        }

        @Override public String toString() {
            return SimpleLock.class.getName()+"("+lock+")";
        }
    }


    private static final class LoggingLock extends SimpleLock {
        private final CallersData callersData = new CallersData();

        @Override void lock() {
            boolean locked = tryLock(logTimeoutMillis, TimeUnit.MILLISECONDS);
            if (!locked) {
                callersData.logBefore();
                super.lock();
                logger.log(logLevel, "Scheduler ThreadLock acquired");
            }
            callersData.remember();
        }

        @Override void unlock() {
//            logger.log(logLevel, "Releasing Scheduler ThreadLock");
            super.unlock();
            callersData.forget();
        }

        private static class CallersData {
            @Nullable private volatile Thread lockingThread = null;
            @Nullable private volatile Exception lockingStackTrace = null;
        
            private synchronized void logBefore() {
                Thread t = lockingThread;
                Exception x = lockingStackTrace;
                StringWriter stringWriter = new StringWriter();
                PrintWriter w = new PrintWriter(stringWriter);
                w.print("Waiting for Scheduler ThreadLock, currently acquired of ");
                w.print(t);
                w.write(", stack trace was:\n");
                if (x != null)  x.printStackTrace(w);
                w.flush();
                logger.log(logLevel, stringWriter.toString());
            }

            private synchronized void remember() {
                lockingThread = Thread.currentThread();
                lockingStackTrace = new Exception();
                lockingStackTrace.fillInStackTrace();
            }

            private synchronized void forget() {
                lockingThread = null;
                lockingStackTrace = null;
            }
        }

//    private static void log(String s) {
//        // Nicht genau, weil synchronize fehlt
//        System.err.println(ThreadLock.class +
//          " " + Thread.currentThread() +
//          " holdCount=" + lock.getHoldCount() +
//          " hasQueuedThread=" + lock.hasQueuedThreads() +
//          " " + s);
//        System.err.flush();
//    }
    }
}
