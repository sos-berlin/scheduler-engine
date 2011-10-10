package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;

/** Schnittstelle zum Start des Schedulers über JNI. */
public class CppScheduler {
    public final void loadModule() {
        System.loadLibrary("scheduler");
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
