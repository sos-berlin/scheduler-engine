package com.sos.scheduler.engine.cplusplus.runtime;

import com.google.common.base.Preconditions;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.ReentrantLock;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import static java.lang.System.currentTimeMillis;
import static java.lang.Thread.currentThread;

public class ThreadLock {
    private static final int logTimeoutMillis = 15*1000;     // Wenn's länger dauert, Meldung loggen
    private static final Logger logger = LoggerFactory.getLogger(ThreadLock.class);

    private final SimpleLock myLock = new LoggingLock();

    public final void lock() {
        myLock.lock();
    }

    public final void unlock() {
        myLock.unlock();
    }

    public final void requireUnlocked() {
        Preconditions.checkState(!myLock.lock.isLocked(), "ThreadLock should not be locked at JobScheduler start");
    }

    @Override public String toString() {
        return myLock.toString();
    }

    private static class SimpleLock {
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

        boolean unlock() {
            lock.unlock();
            return !lock.isHeldByCurrentThread();
        }

        @Override public String toString() {
            return SimpleLock.class.getName()+"("+lock+")";
        }
    }


    private static final class LoggingLock extends SimpleLock {
        private final AtomicReference<Thread> lockingThread = new AtomicReference<>();  // Ungenau wegen race condition

        @Override void lock() {
            long t = currentTimeMillis();
            boolean locked = tryLock(logTimeoutMillis, TimeUnit.MILLISECONDS);
            if (!locked) {
                logger.warn("Waiting for Scheduler ThreadLock, currently acquired by "+lockingThreadString() + "\n" +
                    "Own stack:\n" + stackTraceToString(currentThread()));
                super.lock();
                logger.warn("Scheduler ThreadLock acquired after waiting {}ms", currentTimeMillis() - t);
            }
        }

        private String lockingThreadString() {
            Thread t = lockingThread.get();
            return t == null? "(unknown)" : stackTraceToString(t);
        }

        @Override protected void onLocked() {
            lockingThread.set(currentThread());
        }

        @Override boolean unlock() {
            boolean unlocked = super.unlock();
            if (unlocked) {  // Not in case of nested calls to lock
                lockingThread.compareAndSet(currentThread(), null);
            }
            return unlocked;
        }
    }

    private static String stackTraceToString(Thread t) {
        StringWriter stringWriter = new StringWriter();
        PrintWriter w = new PrintWriter(stringWriter);
        w.print(t);
        w.print(", current stack trace:\n");
        Exception x = new Exception() {};
        x.setStackTrace(t.getStackTrace());
        x.printStackTrace(w);
        w.flush();
        return stringWriter.toString();
    }
}
