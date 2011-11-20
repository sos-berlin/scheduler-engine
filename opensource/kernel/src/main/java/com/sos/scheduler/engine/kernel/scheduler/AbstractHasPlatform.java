package com.sos.scheduler.engine.kernel.scheduler;

import com.sos.scheduler.engine.kernel.log.PrefixLog;


public abstract class AbstractHasPlatform implements HasPlatform {
    private final Platform platform;
    private PrefixLog log = null;


    protected AbstractHasPlatform(Platform platform) {
        this.platform = platform;
    }


    @Override public final PrefixLog log() {
        if (log == null)  log = platform.log();
        return log;
    }

    @Override public final Platform getPlatform() {
        return platform;
    }
}
