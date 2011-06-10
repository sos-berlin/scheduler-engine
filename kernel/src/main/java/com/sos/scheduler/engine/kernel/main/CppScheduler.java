package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;


public class CppScheduler {
    public final void loadModule() {
        System.loadLibrary("scheduler");
    }

    
    public  final int run(String[] arguments, String argumentLine, MainContext javaMainContext) {
        CppProxy.threadLock.lock();
        try {
            return runNative(arguments, argumentLine, javaMainContext);
        }
        finally {
            CppProxy.threadLock.unlock();
        }
    }


    private native int runNative(String[] arguments, String argumentLine, MainContext javaMainContext);
}
