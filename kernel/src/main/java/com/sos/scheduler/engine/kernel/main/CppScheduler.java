package com.sos.scheduler.engine.kernel.main;

import static com.google.common.base.Throwables.propagate;

import java.io.File;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;

/** Schnittstelle zum Start des Schedulers über JNI. */
public class CppScheduler {
    private static final Logger logger = Logger.getLogger(CppScheduler.class);

    public static void loadModuleFromPath() {
        System.loadLibrary("scheduler");
    }

    public final void loadModule(File moduleFile) {
        try {
            logger.debug("Load "+moduleFile+", java.library.path="+System.getProperty("java.library.path"));
            System.load(moduleFile.getPath());
            logger.debug(moduleFile.getName() + " loaded");
        } catch (Throwable t) {
            logger.error("Load "+moduleFile+": "+t);
            logger.error("java.library.path=" + System.getProperty("java.library.path"));
            throw propagate(t);
        }
    }

    public  final int run(String[] arguments, String argumentLine, SchedulerStateHandler javaSchedulerStateHandler) {
        CppProxy.threadLock.lock();
        try {
            return runNative(arguments, argumentLine, javaSchedulerStateHandler);
        }
        finally {
            CppProxy.threadLock.unlock();
        }
    }

    private native int runNative(String[] arguments, String argumentLine, SchedulerStateHandler javaSchedulerStateHandler);
}
