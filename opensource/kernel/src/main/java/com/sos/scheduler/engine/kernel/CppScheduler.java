package com.sos.scheduler.engine.kernel;

import static com.google.common.base.Throwables.propagate;

import java.io.File;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;

/** Schnittstelle zum Start des C++-Teils des JobScheduler über JNI. */
public class CppScheduler {
    private static final Logger logger = Logger.getLogger(CppScheduler.class);
    private static volatile CppScheduler singleton = null;
    private String name = null;

    public static void loadModuleFromPath() {
        System.loadLibrary("scheduler");
    }

    public final void loadModule(File moduleFile) {
        try {
            logger.trace("Load "+moduleFile+", java.library.path="+System.getProperty("java.library.path"));
            System.load(moduleFile.getPath());
            logger.trace(moduleFile.getName() + " loaded");
        } catch (Throwable t) {
            logger.error("Load "+moduleFile+": "+t);
            logger.error("java.library.path=" + System.getProperty("java.library.path"));
            throw propagate(t);
        }
    }

    public final int run(String[] arguments, String argumentLine, SchedulerControllerBridge controllerBridge) {
        name = controllerBridge.getName();
        if (singleton != null)
            throw new RuntimeException("Running "+singleton +" hinders start of "+this);
        singleton = this;
        try {
            return run2(arguments, argumentLine, controllerBridge);
        }
        finally {
            singleton = null;
        }
    }

    private int run2(String[] arguments, String argumentLine, SchedulerControllerBridge controllerBridge) {
        CppProxy.threadLock.lock();
        logger.trace(this +" starts");
        try {
            return runNative(arguments, argumentLine, controllerBridge);
        }
        finally {
            logger.trace(this +" has ended");
            CppProxy.threadLock.unlock();
        }
    }

    @Override public String toString() {
        return "JobScheduler C++ Engine '"+name+"'";
    }

    private native int runNative(String[] arguments, String argumentLine, SchedulerControllerBridge controllerBridge);
}
