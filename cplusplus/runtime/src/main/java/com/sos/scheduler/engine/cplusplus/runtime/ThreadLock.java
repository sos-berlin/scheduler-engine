package com.sos.scheduler.engine.cplusplus.runtime;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;
import org.apache.log4j.Logger;


public class ThreadLock {
    private static final ReentrantLock lock = new ReentrantLock();
    private static final int debugTimeoutMillis = 1000;     // Wenn's länger dauert, Debug-Meldung ausgeben
    private static final Logger logger = Logger.getLogger(ThreadLock.class);
    private static volatile Thread lockingThread = null;
    private static volatile Exception lockingStackTrace = null;


    //TODO public static ThreadLock singleton = new ThreadLock();  Methoden nicht mehr statisch
    //TODO Code aufräumen
    
    public static void lock() {
        if (!logger.isDebugEnabled())
            lock.lock();
        else {
            boolean locked = tryLock(debugTimeoutMillis, TimeUnit.MILLISECONDS);
            if (!locked) {
                Thread t = lockingThread;
                Exception x = lockingStackTrace;
                StringWriter stringWriter = new StringWriter();
                PrintWriter w = new PrintWriter(stringWriter);
                w.print("Waiting for Scheduler ThreadLock, currently acquired of ");
                w.print(t);
                w.write(", stack trace was:\n");
                if (x != null)  x.printStackTrace(w);
                w.flush();
                logger.debug(stringWriter.toString());
                lock.lock();
                logger.debug("Scheduler ThreadLock acquired");
            }
        }
        
        lockingThread = Thread.currentThread();

        lockingStackTrace = new Exception();
        lockingStackTrace.fillInStackTrace();
    }


    private static boolean tryLock(int time, TimeUnit unit) {
        try {
            return lock.tryLock(time, unit);
        }
        catch (InterruptedException x) { throw new RuntimeException(x); }
    }


    public static void unlock() {
//        if (logger.isDebugEnabled() && lock.hasQueuedThreads() )
//            logger.debug("Releasing Scheduler ThreadLock");
        lock.unlock();
        lockingThread = null;
        lockingStackTrace = null;
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
