package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.cplusplus.runtime.ThreadLock;


public class CppScheduler {
    public void loadModule() {
        System.loadLibrary("scheduler");
    }

    
    public int run(String[] arguments, String argumentLine, MainContext javaMainContext) {
        ThreadLock.lock();
        try {
            return runNative(arguments, argumentLine, javaMainContext);
        }
        finally {
            ThreadLock.unlock();
        }
    }


    private native int runNative(String[] arguments, String argumentLine, MainContext javaMainContext);
}
