package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.log.PrefixLog;


public final class NullSchedulerObject implements SchedulerObject {
    public static final NullSchedulerObject singleton = new NullSchedulerObject();
    
    
    private NullSchedulerObject() {}


    @Override public PrefixLog log() {
        throw new UnsupportedOperationException(getClass().getName());
    }


    @Override public String toString() { 
        return getClass().getSimpleName();
    }
}
