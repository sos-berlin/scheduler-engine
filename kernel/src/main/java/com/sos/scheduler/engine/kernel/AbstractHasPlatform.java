package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.log.PrefixLog;


public abstract class AbstractHasPlatform implements HasPlatform {
    protected final Platform platform;
    private PrefixLog log;


    protected AbstractHasPlatform(Platform platform) {
        this.platform = platform;
    }

    
    @Override public final Platform getPlatform() {
        return platform;
    }


    @Override public final PrefixLog log() {
        if (log == null)  log = platform.log();
        return log;
    }
}
