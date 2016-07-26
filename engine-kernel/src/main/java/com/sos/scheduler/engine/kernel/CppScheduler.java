package com.sos.scheduler.engine.kernel;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;
import java.io.File;
import java.util.concurrent.atomic.AtomicReference;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.toArray;

/** Schnittstelle zum Start des C++-Teils des JobScheduler über JNI. */
public class CppScheduler {
    private static final Logger logger = LoggerFactory.getLogger(CppScheduler.class);
    private static final AtomicReference<CppScheduler> onlyInstance = new AtomicReference<CppScheduler>();
    private final boolean isCppThreadRequired;
    private String name = null;

    public CppScheduler(boolean isCppThreadRequired) {
        this.isCppThreadRequired = isCppThreadRequired;
    }

    public static void loadModuleFromPath() {
        System.loadLibrary("jobscheduler-engine");
    }

    public static void loadModule(File moduleFile) {
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

    public final int run(ImmutableList<String> arguments, String argumentLine, SchedulerControllerBridge controllerBridge) {
        name = controllerBridge.getName();
        boolean ok  = onlyInstance.compareAndSet(null, this);
        if (!ok)
            throw new RuntimeException("Running "+ onlyInstance.get() +" blocks start of "+this);
        try {
            return run2(arguments, argumentLine, controllerBridge);
        }
        finally {
            onlyInstance.set(null);
        }
    }

    private int run2(ImmutableList<String> arguments, String argumentLine, SchedulerControllerBridge controllerBridge) {
        CppProxy.threadLock.setCppThreadRequired(isCppThreadRequired);
        CppProxy.threadLock.requireUnlocked();
        CppProxy.threadLock.startLock();
        if (logger.isTraceEnabled()) logger.trace(this +" starts");
        try {
            return runNative(toArray(arguments, String.class), argumentLine, controllerBridge);
        }
        finally {
            if (logger.isTraceEnabled()) logger.trace(this +" has ended");
            CppProxy.threadLock.endLock();
        }
    }

    @Override public String toString() {
        return "JobScheduler C++ Engine '"+name+"'";
    }

    private native int runNative(String[] arguments, String argumentLine, SchedulerControllerBridge controllerBridge);
}
